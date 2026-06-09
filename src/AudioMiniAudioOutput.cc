/** @file
 * miniaudio playback output backend.
 */

#include "config.h"

#if WITH_MINIAUDIO == 1

#include "Audio.h"
#include "ProcessServices.h"

#include <chrono>
#include <stdint.h>
#include <string>
#include <string.h>
#include <thread>

#include "miniaudio.h"

struct AudioMiniAudioOutputState {
    PcmFormat pcmFormatValue;
    AudioOutputConfig outputConfigValue;
    ma_context context;
    ma_device device;
    ma_device_id playbackDeviceID;
    int contextInitialized;
    int deviceInitialized;
    int deviceStarted;
    int playbackDeviceIDValid;
    AudioMiniAudioOutput::MiniAudioDeviceSampleFormat deviceSampleFormat;
    ma_format deviceFormat;
    int deviceChannels;
    int deviceSampleRate;
    int deviceBytesPerFrame;
    int directCopy;
    int requestedPeriodMilliseconds;
    int requestedPeriods;
    std::atomic<AudioOutputStream*> callbackStream;
    std::atomic<const std::atomic<int>*> callbackInputFinished;
    char* callbackScratch;
    int callbackScratchSamples;
    int callbackScratchBytes;
    std::atomic<int> callbackDrainActive;
    std::atomic<int> callbackDrainDone;
    std::atomic<int> callbackInFlight;
    std::atomic<int> miniaudioPresentationDelaySamples;
    std::atomic<int> underflowCountValue;
    int lastReportedUnderflowCount;
    int lastReportedDrainDone;
    long long lastReportedSubmittedEndSample;
    char backendNameValue[64];
    char deviceName[256];

    AudioMiniAudioOutputState(const PcmFormat& format,
        const AudioOutputConfig& config)
        : pcmFormatValue(format)
        , outputConfigValue(config)
        , contextInitialized(0)
        , deviceInitialized(0)
        , deviceStarted(0)
        , playbackDeviceIDValid(0)
        , deviceSampleFormat(
              AudioMiniAudioOutput::MiniAudioDeviceSampleFormatUnknown)
        , deviceFormat(ma_format_unknown)
        , deviceChannels(0)
        , deviceSampleRate(0)
        , deviceBytesPerFrame(0)
        , directCopy(0)
        , requestedPeriodMilliseconds(0)
        , requestedPeriods(0)
        , callbackStream(NULL)
        , callbackInputFinished(NULL)
        , callbackScratch(NULL)
        , callbackScratchSamples(0)
        , callbackScratchBytes(0)
        , callbackDrainActive(0)
        , callbackDrainDone(0)
        , callbackInFlight(0)
        , miniaudioPresentationDelaySamples(0)
        , underflowCountValue(0)
        , lastReportedUnderflowCount(0)
        , lastReportedDrainDone(0)
        , lastReportedSubmittedEndSample(-1) {
        memset(&context, 0, sizeof(context));
        memset(&device, 0, sizeof(device));
        memset(&playbackDeviceID, 0, sizeof(playbackDeviceID));
        strcpy(backendNameValue, "unknown");
        deviceName[0] = '\0';
    }
};

static int audioMiniAudioIsNativeS16Format(int sampleFormat) {
#ifdef WORDS_BIGENDIAN
    return sampleFormat == SF_s16_be;
#else
    return sampleFormat == SF_s16_le;
#endif
}

static AudioMiniAudioOutput::MiniAudioDeviceSampleFormat
audioMiniAudioDeviceSampleFormat(const PcmFormat& format, int* directCopy) {
    *directCopy = 0;

    if (format.sampleFormat == SF_u8) {
        *directCopy = 1;
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatU8;
    }

    if (audioMiniAudioIsNativeS16Format(format.sampleFormat)) {
        *directCopy = 1;
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16;
    }

    return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16;
}

static ma_format audioMiniAudioMaFormat(
    AudioMiniAudioOutput::MiniAudioDeviceSampleFormat format) {
    switch (format) {
    case AudioMiniAudioOutput::MiniAudioDeviceSampleFormatU8:
        return ma_format_u8;
    case AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16:
        return ma_format_s16;
    case AudioMiniAudioOutput::MiniAudioDeviceSampleFormatF32:
        return ma_format_f32;
    default:
        return ma_format_unknown;
    }
}

static AudioMiniAudioOutput::MiniAudioDeviceSampleFormat
audioMiniAudioDeviceSampleFormatFromMa(ma_format format) {
    switch (format) {
    case ma_format_u8:
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatU8;
    case ma_format_s16:
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16;
    case ma_format_f32:
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatF32;
    default:
        return AudioMiniAudioOutput::MiniAudioDeviceSampleFormatUnknown;
    }
}

static int audioMiniAudioCanDirectCopy(const PcmFormat& format,
    ma_format deviceFormat, int deviceChannels) {
    if (deviceChannels != format.channels)
        return 0;

    if (format.sampleFormat == SF_u8)
        return deviceFormat == ma_format_u8;

    if (audioMiniAudioIsNativeS16Format(format.sampleFormat))
        return deviceFormat == ma_format_s16;

    return 0;
}

static ma_format audioMiniAudioDeviceFormat(const PcmFormat& format,
    int* directCopy,
    AudioMiniAudioOutput::MiniAudioDeviceSampleFormat* sampleFormat) {
    AudioMiniAudioOutput::MiniAudioDeviceSampleFormat mapped
        = audioMiniAudioDeviceSampleFormat(format, directCopy);
    if (sampleFormat != NULL)
        *sampleFormat = mapped;
    return audioMiniAudioMaFormat(mapped);
}

static int audioMiniAudioReadU16Le(const unsigned char* sample) {
    return int(sample[0]) | (int(sample[1]) << 8);
}

static int audioMiniAudioReadU16Be(const unsigned char* sample) {
    return (int(sample[0]) << 8) | int(sample[1]);
}

static int audioMiniAudioReadS16(const unsigned char* sample, int sampleFormat) {
    switch (sampleFormat) {
    case SF_u8:
        return (int(sample[0]) - 128) << 8;
    case SF_s8:
        return audioSigned8FromByte(sample[0]) << 8;
    case SF_u16_le:
        return audioMiniAudioReadU16Le(sample) - 32768;
    case SF_s16_le:
        return (int16_t)audioMiniAudioReadU16Le(sample);
    case SF_u16_be:
        return audioMiniAudioReadU16Be(sample) - 32768;
    case SF_s16_be:
        return (int16_t)audioMiniAudioReadU16Be(sample);
    default:
        return 0;
    }
}

static int audioMiniAudioSourceFrameS16(const PcmFormat& format,
    const unsigned char* frameSrc, int outputChannel, int outputChannels) {
    int channels = format.channels;
    int sourceBytesPerChannel = (format.sampleFormat < 2) ? 1 : 2;

    if ((frameSrc == NULL) || (channels <= 0))
        return 0;

    if ((outputChannels == 1) && (channels > 1)) {
        long sum = 0;
        for (int channel = 0; channel < channels; channel++) {
            sum += audioMiniAudioReadS16(
                frameSrc + channel * sourceBytesPerChannel,
                format.sampleFormat);
        }
        return int(sum / channels);
    }

    int sourceChannel = outputChannel;
    if (sourceChannel >= channels) {
        if (channels == 1)
            sourceChannel = 0;
        else
            return 0;
    }

    return audioMiniAudioReadS16(
        frameSrc + sourceChannel * sourceBytesPerChannel,
        format.sampleFormat);
}

static int audioMiniAudioClampInt(int value, int minimum, int maximum) {
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static void audioMiniAudioWriteDeviceSample(ma_format deviceFormat,
    char* dst, int sample) {
    switch (deviceFormat) {
    case ma_format_u8: {
        unsigned char value = (unsigned char)audioMiniAudioClampInt(
            (sample + 32768) >> 8, 0, 255);
        memcpy(dst, &value, sizeof(value));
        break;
    }
    case ma_format_s16: {
        int16_t value = (int16_t)sample;
        memcpy(dst, &value, sizeof(value));
        break;
    }
    case ma_format_f32: {
        float value = (float)sample / 32768.0f;
        memcpy(dst, &value, sizeof(value));
        break;
    }
    default:
        break;
    }
}

static void audioMiniAudioConvertToDevice(const PcmFormat& format,
    ma_format deviceFormat, int deviceChannels, int deviceBytesPerFrame,
    const char* src, void* dst, int frames) {
    int sourceBytesPerFrame = format.bytesPerSample();
    int deviceBytesPerChannel = (int)ma_get_bytes_per_sample(deviceFormat);

    if ((src == NULL) || (dst == NULL) || (frames <= 0)
        || (format.channels <= 0) || (deviceChannels <= 0)
        || (deviceBytesPerFrame <= 0))
        return;

    ma_silence_pcm_frames(dst, (ma_uint64)frames, deviceFormat,
        (ma_uint32)deviceChannels);
    if (deviceBytesPerChannel <= 0)
        return;

    for (int frame = 0; frame < frames; frame++) {
        const unsigned char* frameSrc
            = (const unsigned char*)src + frame * sourceBytesPerFrame;
        char* frameDst = (char*)dst + frame * deviceBytesPerFrame;
        for (int channel = 0; channel < deviceChannels; channel++) {
            int sample = audioMiniAudioSourceFrameS16(format, frameSrc,
                channel, deviceChannels);
            audioMiniAudioWriteDeviceSample(deviceFormat,
                frameDst + channel * deviceBytesPerChannel, sample);
        }
    }
}

static void audioMiniAudioSilence(AudioMiniAudioOutputState* state,
    void* output, unsigned int frameOffset, unsigned int frameCount) {
    if ((state == NULL) || (output == NULL) || (frameCount == 0))
        return;

    char* dst = (char*)output + frameOffset * state->deviceBytesPerFrame;
    ma_silence_pcm_frames(dst, frameCount, state->deviceFormat,
        (ma_uint32)state->deviceChannels);
}

static void audioMiniAudioDrain(AudioMiniAudioOutputState* state,
    void* output, unsigned int frameCount) {
    if ((state == NULL) || (output == NULL) || (frameCount == 0))
        return;

    state->callbackInFlight.fetch_add(1);

    AudioOutputStream* stream = state->callbackStream.load();
    const std::atomic<int>* inputFinished
        = state->callbackInputFinished.load();
    if (!state->callbackDrainActive.load() || (stream == NULL)) {
        audioMiniAudioSilence(state, output, 0, frameCount);
        state->callbackInFlight.fetch_sub(1);
        return;
    }

    unsigned int filled = 0;
    while (filled < frameCount) {
        stream->resyncIfBehind();

        int queued = stream->queuedForOutputSamples();
        if (queued <= 0) {
            if (inputFinished != NULL && inputFinished->load())
                state->callbackDrainDone.store(1);
            else
                state->underflowCountValue.fetch_add(1);
            audioMiniAudioSilence(state, output, filled, frameCount - filled);
            state->callbackInFlight.fetch_sub(1);
            return;
        }

        unsigned int remaining = frameCount - filled;
        int wanted = (queued < int(remaining)) ? queued : int(remaining);
        if (wanted > state->callbackScratchSamples)
            wanted = state->callbackScratchSamples;
        if (wanted <= 0) {
            audioMiniAudioSilence(state, output, filled, frameCount - filled);
            state->callbackInFlight.fetch_sub(1);
            return;
        }

        char* dst = (char*)output + filled * state->deviceBytesPerFrame;
        int samples = 0;
        if (state->directCopy) {
            samples = stream->peekForOutput(dst, wanted);
        } else {
            samples = stream->peekForOutput(state->callbackScratch, wanted);
            if (samples > 0)
                audioMiniAudioConvertToDevice(state->pcmFormatValue,
                    state->deviceFormat, state->deviceChannels,
                    state->deviceBytesPerFrame, state->callbackScratch, dst,
                    samples);
        }

        if (samples <= 0) {
            audioMiniAudioSilence(state, output, filled, frameCount - filled);
            state->callbackInFlight.fetch_sub(1);
            return;
        }

        int committedSamples = stream->commitOutputSamples(samples);
        if (committedSamples < samples) {
            audioMiniAudioSilence(state, output, filled + committedSamples,
                frameCount - filled - committedSamples);
            filled += committedSamples;
            state->callbackInFlight.fetch_sub(1);
            return;
        }

        filled += committedSamples;
    }

    state->callbackInFlight.fetch_sub(1);
}

static void audioMiniAudioDataCallback(ma_device* device, void* output,
    const void*, ma_uint32 frameCount) {
    if (device == NULL) {
        return;
    }

    audioMiniAudioDrain((AudioMiniAudioOutputState*)device->pUserData,
        output, frameCount);
}

static void audioMiniAudioLogPlaybackConnected(
    AudioMiniAudioOutputState* state, LogSink& log) {
    if (state == NULL)
        return;

    log.debug("    audio output strategy: miniaudio playback connected backend=%s device=`%s' rate=%d channels=%d format=%d direct-copy=%d target-latency-ms=%d requested-period-ms=%d requested-periods=%d internal-period-frames=%d internal-periods=%d presentation-delay-samples=%d\n",
        state->backendNameValue,
        state->deviceName[0] ? state->deviceName : "default",
        state->deviceSampleRate, state->deviceChannels,
        int(state->deviceFormat), state->directCopy,
        state->outputConfigValue.miniAudioOutputTargetLatencyMs,
        state->requestedPeriodMilliseconds, state->requestedPeriods,
        int(state->device.playback.internalPeriodSizeInFrames),
        int(state->device.playback.internalPeriods),
        state->miniaudioPresentationDelaySamples.load());
}

static int audioMiniAudioBackendNameIsNull(const char* backendName) {
    return (backendName != NULL) && (strcmp(backendName, "Null") == 0);
}

static int audioMiniAudioRequestedPeriodMilliseconds(int targetLatencyMs) {
    if (targetLatencyMs <= 0)
        return 0;

    int periodMs = targetLatencyMs / 4;
    if (periodMs < 5)
        periodMs = 5;
    if (periodMs > 50)
        periodMs = 50;

    return periodMs;
}

static int audioMiniAudioPresentationDelaySamples(int sampleRate,
    int targetLatencyMs, int internalPeriodFrames, int internalPeriods) {
    (void)internalPeriods;

    // miniaudio/CoreAudio is callback driven: submitted samples are the period
    // currently handed to the device, not an already-filled server queue like
    // PulseAudio. Subtract one callback period so visual frame selection tracks
    // the audible slice instead of lagging by the whole internal period queue.
    if (internalPeriodFrames > 0)
        return internalPeriodFrames;

    if ((sampleRate <= 0) || (targetLatencyMs <= 0))
        return 0;

    return (sampleRate
               * audioMiniAudioRequestedPeriodMilliseconds(targetLatencyMs))
        / 1000;
}

static void audioMiniAudioStoreBackendName(AudioMiniAudioOutputState* state) {
    if (state == NULL)
        return;

    const char* backendName = "unknown";
    if (state->device.pContext != NULL)
        backendName = ma_get_backend_name(state->device.pContext->backend);
    strncpy(state->backendNameValue, backendName,
        sizeof(state->backendNameValue));
    state->backendNameValue[sizeof(state->backendNameValue) - 1] = '\0';
}

static void audioMiniAudioLogPlaybackDeviceCandidates(LogSink& log,
    const ma_device_info* playbackDevices, ma_uint32 playbackDeviceCount) {
    const ma_uint32 deviceLogLimit = 8;
    ma_uint32 devicesToLog = playbackDeviceCount < deviceLogLimit
        ? playbackDeviceCount
        : deviceLogLimit;

    for (ma_uint32 i = 0; i < devicesToLog; i++) {
        log.debug("    audio output strategy: miniaudio available playback device[%u]=`%s'%s\n",
            (unsigned int)i,
            playbackDevices[i].name[0] ? playbackDevices[i].name : "unnamed",
            playbackDevices[i].isDefault ? " (default)" : "");
    }

    if (playbackDeviceCount > devicesToLog) {
        log.debug("    audio output strategy: miniaudio playback device list truncated after %u of %u device(s)\n",
            (unsigned int)devicesToLog,
            (unsigned int)playbackDeviceCount);
    }
}

static int audioMiniAudioResolvePlaybackDeviceId(
    AudioMiniAudioOutputState* state, LogSink& log) {
    if (state == NULL)
        return 1;

    const std::string& requestedDevice
        = state->outputConfigValue.miniAudioPlaybackDeviceName;
    if (requestedDevice.empty())
        return 0;

    ma_result result = ma_context_init(NULL, 0, NULL, &state->context);
    if (result != MA_SUCCESS) {
        log.debug("    audio output strategy: miniaudio failed to initialize context for playback device `%s': %s\n",
            requestedDevice.c_str(), ma_result_description(result));
        return 1;
    }
    state->contextInitialized = 1;

    ma_device_info* playbackDevices = NULL;
    ma_uint32 playbackDeviceCount = 0;
    result = ma_context_get_devices(&state->context, &playbackDevices,
        &playbackDeviceCount, NULL, NULL);
    if (result != MA_SUCCESS) {
        log.debug("    audio output strategy: miniaudio failed to enumerate playback devices for `%s': %s\n",
            requestedDevice.c_str(), ma_result_description(result));
        return 1;
    }

    for (ma_uint32 i = 0; i < playbackDeviceCount; i++) {
        if (strcmp(playbackDevices[i].name, requestedDevice.c_str()) == 0) {
            state->playbackDeviceID = playbackDevices[i].id;
            state->playbackDeviceIDValid = 1;
            log.debug("    audio output strategy: miniaudio selected requested playback device `%s'\n",
                requestedDevice.c_str());
            return 0;
        }
    }

    log.debug("    audio output strategy: miniaudio playback device `%s' was not found among %u device(s)\n",
        requestedDevice.c_str(), (unsigned int)playbackDeviceCount);
    audioMiniAudioLogPlaybackDeviceCandidates(log, playbackDevices,
        playbackDeviceCount);
    return 1;
}

AudioMiniAudioOutput::AudioMiniAudioOutput(const PcmFormat& format,
    SecondsClock& clock_, LogSink& log_, const AudioOutputConfig& config,
    AudioOutputDump* outputDump, int autoOpen)
    : AudioOutput(config.miniAudioOutputTargetLatencyMs, outputDump, clock_,
        log_)
    , state(new AudioMiniAudioOutputState(format, config)) {
    if (autoOpen)
        open(format);
}

AudioMiniAudioOutput::~AudioMiniAudioOutput() {
    close();
    delete[] state->callbackScratch;
    state->callbackScratch = NULL;
    delete state;
    state = NULL;
}

void AudioMiniAudioOutput::open(const PcmFormat& format) {
    close();

    if (format.sampleRate <= 0 || format.channels <= 0) {
        logSink().debug("    audio output strategy: miniaudio unavailable for invalid PCM format rate=%d channels=%d format=%d\n",
            format.sampleRate, format.channels, format.sampleFormat);
        return;
    }

    state->pcmFormatValue = format;
    state->deviceFormat = audioMiniAudioDeviceFormat(format,
        &state->directCopy, &state->deviceSampleFormat);
    state->deviceChannels = format.channels;
    state->deviceSampleRate = format.sampleRate;
    state->deviceBytesPerFrame = (int)ma_get_bytes_per_frame(
        state->deviceFormat, (ma_uint32)state->deviceChannels);
    state->requestedPeriodMilliseconds = 0;
    state->requestedPeriods = 0;
    state->playbackDeviceIDValid = 0;
    memset(&state->playbackDeviceID, 0, sizeof(state->playbackDeviceID));
    state->callbackDrainDone.store(0);
    state->underflowCountValue.store(0);
    state->lastReportedUnderflowCount = 0;
    state->lastReportedDrainDone = 0;
    state->lastReportedSubmittedEndSample = -1;
    strcpy(state->backendNameValue, "unknown");
    state->deviceName[0] = '\0';

    ma_device_config deviceConfig = ma_device_config_init(
        ma_device_type_playback);
    deviceConfig.sampleRate = (ma_uint32)format.sampleRate;
    deviceConfig.playback.format = state->deviceFormat;
    deviceConfig.playback.channels = (ma_uint32)format.channels;
    deviceConfig.dataCallback = audioMiniAudioDataCallback;
    deviceConfig.pUserData = state;

    int targetMs = state->outputConfigValue.miniAudioOutputTargetLatencyMs;
    if (targetMs > 0) {
        int periodMs = audioMiniAudioRequestedPeriodMilliseconds(targetMs);
        deviceConfig.periodSizeInMilliseconds = (ma_uint32)periodMs;
        deviceConfig.periods = 4;
        state->requestedPeriodMilliseconds = periodMs;
        state->requestedPeriods = 4;
    }

    if (audioMiniAudioResolvePlaybackDeviceId(state, logSink())) {
        close();
        return;
    }

    if (state->playbackDeviceIDValid)
        deviceConfig.playback.pDeviceID = &state->playbackDeviceID;

    ma_context* context = state->contextInitialized ? &state->context : NULL;
    ma_result result = ma_device_init(context, &deviceConfig, &state->device);
    if (result != MA_SUCCESS) {
        logSink().debug("    audio output strategy: miniaudio failed to open playback device: %s\n",
            ma_result_description(result));
        return;
    }

    state->deviceInitialized = 1;
    state->deviceFormat = state->device.playback.format;
    state->deviceChannels = (int)state->device.playback.channels;
    state->deviceSampleRate = (int)state->device.sampleRate;
    state->deviceBytesPerFrame = (int)ma_get_bytes_per_frame(
        state->deviceFormat, (ma_uint32)state->deviceChannels);
    state->deviceSampleFormat = audioMiniAudioDeviceSampleFormatFromMa(
        state->deviceFormat);
    state->directCopy = audioMiniAudioCanDirectCopy(format,
        state->deviceFormat, state->deviceChannels);
    if ((state->deviceBytesPerFrame <= 0)
        || (state->deviceSampleFormat
            == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatUnknown)) {
        logSink().debug("    audio output strategy: miniaudio unsupported playback callback format=%d channels=%d\n",
            int(state->deviceFormat), state->deviceChannels);
        close();
        return;
    }

    ma_device_get_name(&state->device, ma_device_type_playback,
        state->deviceName, sizeof(state->deviceName), NULL);
    audioMiniAudioStoreBackendName(state);
    if (audioMiniAudioBackendNameIsNull(state->backendNameValue)) {
        logSink().debug("    audio output strategy: miniaudio opened Null playback backend; no real playback device available\n");
        close();
        return;
    }

    int configuredDelaySamples = audioMiniAudioPresentationDelaySamples(
        state->deviceSampleRate,
        state->outputConfigValue.miniAudioOutputTargetLatencyMs,
        int(state->device.playback.internalPeriodSizeInFrames),
        int(state->device.playback.internalPeriods));
    state->miniaudioPresentationDelaySamples.store(configuredDelaySamples);

    result = ma_device_start(&state->device);
    if (result != MA_SUCCESS) {
        logSink().debug("    audio output strategy: miniaudio failed to start playback device `%s': %s\n",
            state->deviceName[0] ? state->deviceName : "default",
            ma_result_description(result));
        close();
        return;
    }

    state->deviceStarted = 1;

    audioMiniAudioLogPlaybackConnected(state, logSink());
}

void AudioMiniAudioOutput::startCallbackDrainForStream(
    AudioOutputStream& stream, const std::atomic<int>* inputFinished,
    int scratchSamples) {
    int samples = scratchSamples;
    if (samples <= 0)
        samples = targetDelaySamples();
    if (samples <= 0)
        samples = 1024;

    int scratchBytes = pcmBytesForSamples(samples, stream.bytesPerSample());
    char* scratch = new char[scratchBytes];

    stopCallbackDrain();

    delete[] state->callbackScratch;
    state->callbackScratch = scratch;
    state->callbackScratchSamples = samples;
    state->callbackScratchBytes = scratchBytes;
    state->callbackStream.store(&stream);
    state->callbackInputFinished.store(inputFinished);
    state->callbackDrainDone.store(0);
    state->callbackDrainActive.store(1);
}

void AudioMiniAudioOutput::close() {
    if (state == NULL)
        return;

    stopCallbackDrain();

    if (state->deviceStarted) {
        logSink().debug("audio runtime: miniaudio stopping playback device `%s' underflows=%d\n",
            state->deviceName[0] ? state->deviceName : "default",
            state->underflowCountValue.load());
        ma_device_stop(&state->device);
        state->deviceStarted = 0;
    }

    if (state->deviceInitialized) {
        ma_device_uninit(&state->device);
        state->deviceInitialized = 0;
        memset(&state->device, 0, sizeof(state->device));
        state->deviceName[0] = '\0';
    }

    if (state->contextInitialized) {
        ma_context_uninit(&state->context);
        state->contextInitialized = 0;
        memset(&state->context, 0, sizeof(state->context));
        state->playbackDeviceIDValid = 0;
        memset(&state->playbackDeviceID, 0, sizeof(state->playbackDeviceID));
    }
}

void AudioMiniAudioOutput::drainCallback(void* output,
    unsigned int frameCount) {
    audioMiniAudioDrain(state, output, frameCount);
}

int AudioMiniAudioOutput::defaultTargetLatencyMs() const {
    return AudioOutput::defaultTargetLatencyMs();
}

int AudioMiniAudioOutput::timingScratchSamples(int, int targetDelaySamples) const {
    return targetDelaySamples;
}

int AudioMiniAudioOutput::write(const void*, int) {
    return 0;
}

int AudioMiniAudioOutput::isOpen() const {
    return state != NULL && state->deviceStarted;
}

int AudioMiniAudioOutput::isRealtime() const {
    return 1;
}

void AudioMiniAudioOutput::update() {
    if (state == NULL)
        return;

    AudioOutputStream* stream = state->callbackStream.load();
    int underflows = state->underflowCountValue.load();
    int drainDone = state->callbackDrainDone.load();
    long long submittedEndSample = stream != NULL
        ? stream->submittedEndPosition()
        : state->lastReportedSubmittedEndSample;
    int queuedSamples = stream != NULL ? stream->queuedForOutputSamples() : 0;

    if (underflows != state->lastReportedUnderflowCount) {
        logSink().debug("audio runtime: miniaudio callback underflows=%d queued-samples=%d submitted-end-sample=%lld active=%d\n",
            underflows, queuedSamples, submittedEndSample,
            state->callbackDrainActive.load());
        state->lastReportedUnderflowCount = underflows;
    }

    if ((drainDone != state->lastReportedDrainDone)
        || ((submittedEndSample != state->lastReportedSubmittedEndSample)
            && logSink().enabled(5))) {
        logSink().trace("audio runtime",
            "miniaudio callback state active=%d drained=%d queued-samples=%d submitted-end-sample=%lld presentation-delay-samples=%d underflows=%d\n",
            state->callbackDrainActive.load(), drainDone, queuedSamples,
            submittedEndSample, presentationDelaySamples(), underflows);
        state->lastReportedDrainDone = drainDone;
        state->lastReportedSubmittedEndSample = submittedEndSample;
    }
}

int AudioMiniAudioOutput::supportsCallbackDrain() const {
    return isOpen();
}

void AudioMiniAudioOutput::startCallbackDrain(AudioOutputStream& stream,
    const std::atomic<int>* inputFinished, int scratchSamples) {
    if (!isOpen())
        return;

    startCallbackDrainForStream(stream, inputFinished, scratchSamples);

    logSink().debug("audio runtime: miniaudio callback drain started scratch-samples=%d target-buffer-samples=%d queued-samples=%d input-finished=%d\n",
        state->callbackScratchSamples, queuedTargetSamples(),
        stream.queuedForOutputSamples(),
        inputFinished ? inputFinished->load() : 0);
}

void AudioMiniAudioOutput::notifyCallbackDrain() {
    update();
}

void AudioMiniAudioOutput::stopCallbackDrain() {
    if (state == NULL)
        return;

    int wasActive = state->callbackDrainActive.load()
        || (state->callbackStream.load() != NULL);
    state->callbackDrainActive.store(0);
    while (state->callbackInFlight.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    state->callbackStream.store(NULL);
    state->callbackInputFinished.store(NULL);
    state->callbackScratchSamples = 0;
    state->callbackScratchBytes = 0;
    if (wasActive) {
        logSink().debug("audio runtime: miniaudio callback drain stopped drained=%d underflows=%d\n",
            state->callbackDrainDone.load(), state->underflowCountValue.load());
    }
}

int AudioMiniAudioOutput::callbackDrainComplete(const AudioOutputStream& stream,
    int inputFinished) const {
    if ((state == NULL) || !state->callbackDrainActive.load())
        return 0;

    return inputFinished && state->callbackDrainDone.load()
        && (stream.queuedForOutputSamples() == 0);
}

int AudioMiniAudioOutput::presentationDelaySamples() const {
    if (state != NULL) {
        int samples = state->miniaudioPresentationDelaySamples.load();
        if (samples > 0)
            return samples;
    }

    return AudioOutput::presentationDelaySamples();
}

int AudioMiniAudioOutput::queuedTargetSamples() const {
    return AudioOutput::queuedTargetSamples();
}

int AudioMiniAudioOutput::underflowCount() const {
    return state != NULL ? state->underflowCountValue.load() : 0;
}

const char* AudioMiniAudioOutput::backendName() const {
    return state != NULL ? state->backendNameValue : "unknown";
}

#ifdef AUDIO_OUTPUT_TEST_HOOKS
AudioMiniAudioOutput::MiniAudioDeviceSampleFormat
AudioMiniAudioOutput::testDeviceSampleFormatFor(const PcmFormat& format,
    int* directCopy) {
    return audioMiniAudioDeviceSampleFormat(format, directCopy);
}

int AudioMiniAudioOutput::testBackendNameIsNull(const char* backendName) {
    return audioMiniAudioBackendNameIsNull(backendName);
}

int AudioMiniAudioOutput::testPresentationDelaySamples(int sampleRate,
    int targetLatencyMs, int internalPeriodFrames, int internalPeriods) {
    return audioMiniAudioPresentationDelaySamples(sampleRate, targetLatencyMs,
        internalPeriodFrames, internalPeriods);
}

void AudioMiniAudioOutput::testStartCallbackDrainWithoutDevice(
    AudioOutputStream& stream, const std::atomic<int>* inputFinished,
    int scratchSamples) {
    if (state == NULL)
        return;

    int directCopy = 0;
    MiniAudioDeviceSampleFormat callbackFormat
        = audioMiniAudioDeviceSampleFormat(state->pcmFormatValue,
            &directCopy);
    testStartCallbackDrainWithoutDeviceFormat(stream, inputFinished,
        scratchSamples, callbackFormat, state->pcmFormatValue.channels);
}

void AudioMiniAudioOutput::testStartCallbackDrainWithoutDeviceFormat(
    AudioOutputStream& stream, const std::atomic<int>* inputFinished,
    int scratchSamples, MiniAudioDeviceSampleFormat callbackFormat,
    int callbackChannels) {
    if (state == NULL)
        return;

    if (callbackChannels <= 0)
        callbackChannels = state->pcmFormatValue.channels;
    state->deviceSampleFormat = callbackFormat;
    state->deviceFormat = audioMiniAudioMaFormat(callbackFormat);
    state->deviceChannels = callbackChannels;
    state->deviceSampleRate = state->pcmFormatValue.sampleRate;
    state->deviceBytesPerFrame = (int)ma_get_bytes_per_frame(
        state->deviceFormat, (ma_uint32)state->deviceChannels);
    state->directCopy = audioMiniAudioCanDirectCopy(state->pcmFormatValue,
        state->deviceFormat, state->deviceChannels);

    startCallbackDrainForStream(stream, inputFinished, scratchSamples);
}

void AudioMiniAudioOutput::testDrainCallback(void* output,
    unsigned int frameCount) {
    drainCallback(output, frameCount);
}

void AudioMiniAudioOutput::testLogConnectedDiagnostics(
    const char* backendName, const char* deviceName, int sampleRate,
    int channels, MiniAudioDeviceSampleFormat callbackFormat, int directCopy,
    int requestedPeriodMilliseconds, int requestedPeriods,
    int internalPeriodFrames, int internalPeriods,
    int presentationDelaySamples) {
    if (state == NULL)
        return;

    strncpy(state->backendNameValue,
        backendName != NULL ? backendName : "unknown",
        sizeof(state->backendNameValue));
    state->backendNameValue[sizeof(state->backendNameValue) - 1] = '\0';
    strncpy(state->deviceName, deviceName != NULL ? deviceName : "",
        sizeof(state->deviceName));
    state->deviceName[sizeof(state->deviceName) - 1] = '\0';
    state->deviceSampleRate = sampleRate;
    state->deviceChannels = channels;
    state->deviceSampleFormat = callbackFormat;
    state->deviceFormat = audioMiniAudioMaFormat(callbackFormat);
    state->directCopy = directCopy;
    state->requestedPeriodMilliseconds = requestedPeriodMilliseconds;
    state->requestedPeriods = requestedPeriods;
    state->device.playback.internalPeriodSizeInFrames
        = (ma_uint32)internalPeriodFrames;
    state->device.playback.internalPeriods = (ma_uint32)internalPeriods;
    state->miniaudioPresentationDelaySamples.store(
        presentationDelaySamples);

    audioMiniAudioLogPlaybackConnected(state, logSink());
}
#endif

#endif
