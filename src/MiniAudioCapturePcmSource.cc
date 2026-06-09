/** @file
 * miniaudio capture source backend.
 */

#include "config.h"

#if WITH_MINIAUDIO == 1

#include "Audio.h"
#include "AudioSettings.h"
#include "ProcessServices.h"

#include <atomic>
#include <stdint.h>
#include <string.h>

#include "miniaudio.h"

struct MiniAudioCapturePcmSourceState {
    PcmFormat requestedFormat;
    ma_context context;
    ma_device device;
    ma_device_id captureDeviceID;
    ma_pcm_rb ring;
    int contextInitialized;
    int ringInitialized;
    int deviceInitialized;
    int deviceStarted;
    int captureDeviceIDValid;
    int bytesPerFrame;
    std::atomic<int> overrunCountValue;
    char requestedDeviceName[PATH_MAX];
    char backendNameValue[64];
    char deviceName[256];

    MiniAudioCapturePcmSourceState()
        : requestedFormat()
        , contextInitialized(0)
        , ringInitialized(0)
        , deviceInitialized(0)
        , deviceStarted(0)
        , captureDeviceIDValid(0)
        , bytesPerFrame(0)
        , overrunCountValue(0) {
        memset(&context, 0, sizeof(context));
        memset(&device, 0, sizeof(device));
        memset(&captureDeviceID, 0, sizeof(captureDeviceID));
        memset(&ring, 0, sizeof(ring));
        requestedDeviceName[0] = '\0';
        strcpy(backendNameValue, "unknown");
        deviceName[0] = '\0';
    }
};

static int miniAudioCaptureIsNativeS16Format(int sampleFormat) {
#ifdef WORDS_BIGENDIAN
    return sampleFormat == SF_s16_be;
#else
    return sampleFormat == SF_s16_le;
#endif
}

static int miniAudioCaptureNativeS16Format() {
#ifdef WORDS_BIGENDIAN
    return SF_s16_be;
#else
    return SF_s16_le;
#endif
}

static ma_format miniAudioCaptureMaFormatForSampleFormat(int sampleFormat) {
    switch (sampleFormat) {
    case SF_u8:
        return ma_format_u8;
    default:
        return ma_format_s16;
    }
}

static int miniAudioCaptureSampleFormatForMaFormat(ma_format format,
    int* sampleFormat) {
    switch (format) {
    case ma_format_u8:
        *sampleFormat = SF_u8;
        return 0;
    case ma_format_s16:
        *sampleFormat = miniAudioCaptureNativeS16Format();
        return 0;
    default:
        return 1;
    }
}

static void miniAudioCaptureApplyNegotiatedFormat(PcmFormat& format) {
    if (format.sampleRate <= 0)
        format.sampleRate = 44100;
    if (format.channels <= 0)
        format.channels = 2;
    if (format.channels > 2)
        format.channels = 2;

    if (format.sampleFormat == SF_u8)
        return;
    if (miniAudioCaptureIsNativeS16Format(format.sampleFormat))
        return;

    format.sampleFormat = miniAudioCaptureNativeS16Format();
}

static void miniAudioCaptureUseRequestedFormat(
    MiniAudioCapturePcmSourceState* state, PcmFormat& pcmFormat) {
    if (state == NULL)
        return;

    pcmFormat = state->requestedFormat;
    miniAudioCaptureApplyNegotiatedFormat(pcmFormat);
    state->bytesPerFrame = pcmFormat.bytesPerSample();
}

static void miniAudioCaptureDataCallback(ma_device* device, void*,
    const void* input, ma_uint32 frameCount) {
    if ((device == NULL) || (input == NULL) || (frameCount == 0))
        return;

    MiniAudioCapturePcmSource* source
        = (MiniAudioCapturePcmSource*)device->pUserData;
    if (source != NULL)
        source->writeCapturedFrames(input, frameCount);
}

static int miniAudioCaptureDefaultRingFrames(const PcmFormat& format) {
    int ringFrames = format.sampleRate / 2;
    if (ringFrames < 4096)
        ringFrames = 4096;
    return ringFrames;
}

static int miniAudioCaptureInitializeRing(MiniAudioCapturePcmSourceState* state,
    const PcmFormat& format, LogSink& log, int ringFrames) {
    if (state == NULL)
        return 1;
    if (ringFrames <= 0)
        ringFrames = miniAudioCaptureDefaultRingFrames(format);

    ma_format maFormat = miniAudioCaptureMaFormatForSampleFormat(
        format.sampleFormat);
    ma_result result = ma_pcm_rb_init(maFormat, (ma_uint32)format.channels,
        (ma_uint32)ringFrames, NULL, NULL, &state->ring);
    if (result != MA_SUCCESS) {
        log.debug("    audio input strategy: miniaudio capture ring init failed: %s\n",
            ma_result_description(result));
        return 1;
    }

    state->ringInitialized = 1;
    state->bytesPerFrame = format.bytesPerSample();
    ma_pcm_rb_set_sample_rate(&state->ring, (ma_uint32)format.sampleRate);
    return 0;
}

static void miniAudioCaptureLogConnected(
    MiniAudioCapturePcmSourceState* state, const PcmFormat& format,
    LogSink& log, int ringFrames) {
    if (state == NULL)
        return;

    log.debug("    audio input strategy: miniaudio capture connected backend=%s device=`%s' rate=%d channels=%d format=%d ring-frames=%d\n",
        state->backendNameValue,
        state->deviceName[0] ? state->deviceName : "default",
        format.sampleRate, format.channels, format.sampleFormat,
        ringFrames);
}

static int miniAudioCaptureBackendNameIsNull(const char* backendName) {
    return (backendName != NULL) && (strcmp(backendName, "Null") == 0);
}

static void miniAudioCaptureStoreBackendName(
    MiniAudioCapturePcmSourceState* state) {
    if (state == NULL)
        return;

    const char* backendName = "unknown";
    if (state->device.pContext != NULL)
        backendName = ma_get_backend_name(state->device.pContext->backend);
    strncpy(state->backendNameValue, backendName,
        sizeof(state->backendNameValue));
    state->backendNameValue[sizeof(state->backendNameValue) - 1] = '\0';
}

static void miniAudioCaptureLogDeviceCandidates(LogSink& log,
    const ma_device_info* captureDevices, ma_uint32 captureDeviceCount) {
    const ma_uint32 deviceLogLimit = 8;
    ma_uint32 devicesToLog = captureDeviceCount < deviceLogLimit
        ? captureDeviceCount
        : deviceLogLimit;

    for (ma_uint32 i = 0; i < devicesToLog; i++) {
        log.debug("    audio input strategy: miniaudio available capture device[%u]=`%s'%s\n",
            (unsigned int)i,
            captureDevices[i].name[0] ? captureDevices[i].name : "unnamed",
            captureDevices[i].isDefault ? " (default)" : "");
    }

    if (captureDeviceCount > devicesToLog) {
        log.debug("    audio input strategy: miniaudio capture device list truncated after %u of %u device(s)\n",
            (unsigned int)devicesToLog,
            (unsigned int)captureDeviceCount);
    }
}

static int miniAudioCaptureResolveDeviceId(
    MiniAudioCapturePcmSourceState* state, LogSink& log) {
    if (state == NULL)
        return 1;
    if (state->requestedDeviceName[0] == '\0')
        return 0;

    ma_result result = ma_context_init(NULL, 0, NULL, &state->context);
    if (result != MA_SUCCESS) {
        log.debug("    audio input strategy: miniaudio failed to initialize context for capture device `%s': %s\n",
            state->requestedDeviceName, ma_result_description(result));
        return 1;
    }
    state->contextInitialized = 1;

    ma_device_info* captureDevices = NULL;
    ma_uint32 captureDeviceCount = 0;
    result = ma_context_get_devices(&state->context, NULL, NULL,
        &captureDevices, &captureDeviceCount);
    if (result != MA_SUCCESS) {
        log.debug("    audio input strategy: miniaudio failed to enumerate capture devices for `%s': %s\n",
            state->requestedDeviceName, ma_result_description(result));
        return 1;
    }

    for (ma_uint32 i = 0; i < captureDeviceCount; i++) {
        if (strcmp(captureDevices[i].name, state->requestedDeviceName) == 0) {
            state->captureDeviceID = captureDevices[i].id;
            state->captureDeviceIDValid = 1;
            log.debug("    audio input strategy: miniaudio selected requested capture device `%s'\n",
                state->requestedDeviceName);
            return 0;
        }
    }

    log.debug("    audio input strategy: miniaudio capture device `%s' was not found among %u device(s)\n",
        state->requestedDeviceName, (unsigned int)captureDeviceCount);
    miniAudioCaptureLogDeviceCandidates(log, captureDevices,
        captureDeviceCount);
    return 1;
}

MiniAudioCapturePcmSource::MiniAudioCapturePcmSource(
    const AudioSettings& settings, LogSink& log_, int autoOpen)
    : PcmSource(log_)
    , state(new MiniAudioCapturePcmSourceState()) {
    state->requestedFormat = settings.pcmFormat;
    strncpy(state->requestedDeviceName, settings.miniAudioCaptureDeviceName,
        sizeof(state->requestedDeviceName));
    state->requestedDeviceName[sizeof(state->requestedDeviceName) - 1] = '\0';
    miniAudioCaptureUseRequestedFormat(state, pcmFormat);

    if (autoOpen && open())
        error = 1;
}

MiniAudioCapturePcmSource::~MiniAudioCapturePcmSource() {
    close();
    delete state;
    state = NULL;
}

int MiniAudioCapturePcmSource::open() {
    close();
    error = 0;
    miniAudioCaptureUseRequestedFormat(state, pcmFormat);
    state->captureDeviceIDValid = 0;
    memset(&state->captureDeviceID, 0, sizeof(state->captureDeviceID));

    ma_format format = miniAudioCaptureMaFormatForSampleFormat(
        pcmFormat.sampleFormat);
    state->bytesPerFrame = pcmFormat.bytesPerSample();

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.sampleRate = (ma_uint32)pcmFormat.sampleRate;
    deviceConfig.capture.format = format;
    deviceConfig.capture.channels = (ma_uint32)pcmFormat.channels;
    deviceConfig.dataCallback = miniAudioCaptureDataCallback;
    deviceConfig.pUserData = this;

    if (miniAudioCaptureResolveDeviceId(state, log)) {
        close();
        return 1;
    }

    if (state->captureDeviceIDValid)
        deviceConfig.capture.pDeviceID = &state->captureDeviceID;

    ma_context* context = state->contextInitialized ? &state->context : NULL;
    ma_result result = ma_device_init(context, &deviceConfig, &state->device);
    if (result != MA_SUCCESS) {
        log.debug("    audio input strategy: miniaudio failed to open capture device: %s\n",
            ma_result_description(result));
        close();
        return 1;
    }

    state->deviceInitialized = 1;
    int deviceSampleFormat = pcmFormat.sampleFormat;
    if (miniAudioCaptureSampleFormatForMaFormat(state->device.capture.format,
            &deviceSampleFormat)) {
        log.debug("    audio input strategy: miniaudio unsupported capture callback format=%d channels=%d\n",
            int(state->device.capture.format),
            int(state->device.capture.channels));
        close();
        return 1;
    }
    if ((state->device.capture.channels == 0)
        || (state->device.capture.channels > 2)
        || (state->device.sampleRate == 0)) {
        log.debug("    audio input strategy: miniaudio unsupported capture callback shape rate=%d channels=%d format=%d\n",
            int(state->device.sampleRate),
            int(state->device.capture.channels),
            int(state->device.capture.format));
        close();
        return 1;
    }

    pcmFormat.sampleFormat = deviceSampleFormat;
    pcmFormat.channels = (int)state->device.capture.channels;
    pcmFormat.sampleRate = (int)state->device.sampleRate;
    state->bytesPerFrame = pcmFormat.bytesPerSample();

    int ringFrames = miniAudioCaptureDefaultRingFrames(pcmFormat);
    if (miniAudioCaptureInitializeRing(state, pcmFormat, log, ringFrames)) {
        close();
        return 1;
    }

    ma_device_get_name(&state->device, ma_device_type_capture,
        state->deviceName, sizeof(state->deviceName), NULL);
    miniAudioCaptureStoreBackendName(state);
    if (miniAudioCaptureBackendNameIsNull(state->backendNameValue)) {
        log.debug("    audio input strategy: miniaudio opened Null capture backend; no real capture device available\n");
        close();
        return 1;
    }

    result = ma_device_start(&state->device);
    if (result != MA_SUCCESS) {
        log.debug("    audio input strategy: miniaudio failed to start capture device `%s': %s\n",
            state->deviceName[0] ? state->deviceName : "default",
            ma_result_description(result));
        close();
        return 1;
    }
    state->deviceStarted = 1;

    miniAudioCaptureLogConnected(state, pcmFormat, log, ringFrames);
    return 0;
}

void MiniAudioCapturePcmSource::close() {
    if (state == NULL)
        return;

    if (state->deviceStarted) {
        log.debug("audio runtime: miniaudio stopping capture device `%s' overruns=%d\n",
            state->deviceName[0] ? state->deviceName : "default",
            state->overrunCountValue.load());
        ma_device_stop(&state->device);
        state->deviceStarted = 0;
    }

    if (state->deviceInitialized) {
        ma_device_uninit(&state->device);
        state->deviceInitialized = 0;
        memset(&state->device, 0, sizeof(state->device));
        state->deviceName[0] = '\0';
    }

    if (state->ringInitialized) {
        ma_pcm_rb_uninit(&state->ring);
        state->ringInitialized = 0;
        memset(&state->ring, 0, sizeof(state->ring));
    }

    if (state->contextInitialized) {
        ma_context_uninit(&state->context);
        state->contextInitialized = 0;
        memset(&state->context, 0, sizeof(state->context));
        state->captureDeviceIDValid = 0;
        memset(&state->captureDeviceID, 0, sizeof(state->captureDeviceID));
    }
}

void MiniAudioCapturePcmSource::writeCapturedFrames(const void* input,
    unsigned int frames) {
    if ((state == NULL) || !state->ringInitialized || (input == NULL)
        || (frames == 0))
        return;

    const char* src = (const char*)input;
    unsigned int remaining = frames;
    while (remaining > 0) {
        ma_uint32 writable = remaining;
        void* dst = NULL;
        ma_result result = ma_pcm_rb_acquire_write(&state->ring, &writable,
            &dst);
        if ((result != MA_SUCCESS) || (writable == 0) || (dst == NULL)) {
            state->overrunCountValue.fetch_add(1);
            return;
        }

        memcpy(dst, src, (size_t)writable * (size_t)state->bytesPerFrame);
        ma_pcm_rb_commit_write(&state->ring, writable);
        src += (size_t)writable * (size_t)state->bytesPerFrame;
        remaining -= writable;
    }
}

int MiniAudioCapturePcmSource::read(char* dst, int rawSize,
    int samplesRequested) {
    if ((state == NULL) || !state->ringInitialized || (dst == NULL)
        || (rawSize <= 0) || (samplesRequested <= 0))
        return 0;

    int maxSamples = rawSize / state->bytesPerFrame;
    int wanted = samplesRequested < maxSamples ? samplesRequested : maxSamples;
    if (wanted <= 0)
        return 0;

    int total = 0;
    while (total < wanted) {
        ma_uint32 readable = (ma_uint32)(wanted - total);
        void* src = NULL;
        ma_result result = ma_pcm_rb_acquire_read(&state->ring, &readable,
            &src);
        if ((result != MA_SUCCESS) || (readable == 0) || (src == NULL))
            break;

        memcpy(dst + (size_t)total * (size_t)state->bytesPerFrame, src,
            (size_t)readable * (size_t)state->bytesPerFrame);
        ma_pcm_rb_commit_read(&state->ring, readable);
        total += (int)readable;
    }

    return total;
}

int MiniAudioCapturePcmSource::rawBufferSize(int frameRawSize,
    int samplesRequested) const {
    if ((state == NULL) || (state->bytesPerFrame <= 0))
        return frameRawSize;

    int captureBytes = pcmBytesForSamples(samplesRequested,
        state->bytesPerFrame);
    return captureBytes > frameRawSize ? captureBytes : frameRawSize;
}

void MiniAudioCapturePcmSource::update() {
    error = open() ? 1 : 0;
}

int MiniAudioCapturePcmSource::isOpen() const {
    return state != NULL && state->deviceStarted;
}

int MiniAudioCapturePcmSource::overrunCount() const {
    return state != NULL ? state->overrunCountValue.load() : 0;
}

const char* MiniAudioCapturePcmSource::backendName() const {
    return state != NULL ? state->backendNameValue : "unknown";
}

#ifdef AUDIO_CAPTURE_TEST_HOOKS
int MiniAudioCapturePcmSource::testBackendNameIsNull(const char* backendName) {
    return miniAudioCaptureBackendNameIsNull(backendName);
}

void MiniAudioCapturePcmSource::testInitializeRingWithoutDevice(int ringFrames) {
    close();
    error = 0;
    miniAudioCaptureUseRequestedFormat(state, pcmFormat);
    miniAudioCaptureInitializeRing(state, pcmFormat, log, ringFrames);
}

void MiniAudioCapturePcmSource::testInitializeRingWithoutDeviceFormat(
    int sampleRate, int channels, int sampleFormat, int ringFrames) {
    close();
    error = 0;
    pcmFormat.sampleRate = sampleRate;
    pcmFormat.channels = channels;
    pcmFormat.sampleFormat = sampleFormat;
    miniAudioCaptureApplyNegotiatedFormat(pcmFormat);
    state->bytesPerFrame = pcmFormat.bytesPerSample();
    miniAudioCaptureInitializeRing(state, pcmFormat, log, ringFrames);
}

void MiniAudioCapturePcmSource::testWriteCapturedFrames(const void* input,
    unsigned int frames) {
    writeCapturedFrames(input, frames);
}

void MiniAudioCapturePcmSource::testLogConnectedDiagnostics(
    const char* backendName, const char* deviceName, int sampleRate,
    int channels, int sampleFormat, int ringFrames) {
    if (state == NULL)
        return;

    strncpy(state->backendNameValue,
        backendName != NULL ? backendName : "unknown",
        sizeof(state->backendNameValue));
    state->backendNameValue[sizeof(state->backendNameValue) - 1] = '\0';
    strncpy(state->deviceName, deviceName != NULL ? deviceName : "",
        sizeof(state->deviceName));
    state->deviceName[sizeof(state->deviceName) - 1] = '\0';

    pcmFormat.sampleRate = sampleRate;
    pcmFormat.channels = channels;
    pcmFormat.sampleFormat = sampleFormat;
    miniAudioCaptureLogConnected(state, pcmFormat, log, ringFrames);
}

int MiniAudioCapturePcmSource::testAvailableReadFrames() const {
    if ((state == NULL) || !state->ringInitialized)
        return 0;
    return (int)ma_pcm_rb_available_read(&state->ring);
}

void MiniAudioCapturePcmSource::testSetError(int value) {
    error = value ? 1 : 0;
}
#endif

#endif
