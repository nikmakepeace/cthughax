/** @file
 * Unit coverage for explicit audio output config propagation.
 */

#include "Audio.h"
#include "AudioSettings.h"
#include "Configuration.h"
#include "ProcessServices.h"
#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if WITH_MINIAUDIO == 1
#ifndef CTH_AUDIO_FIXTURE_DIR
#error CTH_AUDIO_FIXTURE_DIR must point at tests/fixtures/audio
#endif
#endif

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
const char* audioSampleFormatText(int) { return "signed-16"; }

class FakeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

class CapturingLogSink : public LogSink {
    char text[8192];

protected:
    virtual void write(int, const char*, int, const char* fmt, va_list args) {
        size_t used = strlen(text);
        if (used >= sizeof(text) - 1)
            return;
        vsnprintf(text + used, sizeof(text) - used, fmt, args);
    }

public:
    CapturingLogSink() { text[0] = '\0'; }

    virtual int enabled(int) const {
        return 1;
    }

    int contains(const char* token) const {
        return strstr(text, token) != NULL;
    }
};

static PcmFormat stereo16Format() {
    PcmFormat format;
    format.sampleRate = 48000;
    format.channels = 2;
    format.sampleFormat = SF_s16_le;
    return format;
}

static PcmFormat formatFor(int sampleRate, int channels, int sampleFormat) {
    PcmFormat format;
    format.sampleRate = sampleRate;
    format.channels = channels;
    format.sampleFormat = sampleFormat;
    return format;
}

static PcmFormat unsigned8MonoFormat() {
    return formatFor(1000, 1, SF_u8);
}

static PcmFormat signed8MonoFormat() {
    return formatFor(1000, 1, SF_s8);
}

static PcmFormat unsigned16LeMonoFormat() {
    return formatFor(1000, 1, SF_u16_le);
}

static void fixturePath(char* path, size_t size, const char* fileName) {
#if WITH_MINIAUDIO == 1
    snprintf(path, size, "%s/%s", CTH_AUDIO_FIXTURE_DIR, fileName);
    path[size - 1] = '\0';
#else
    (void)path;
    (void)size;
    (void)fileName;
#endif
}

static void testNullOutputReceivesTargetLatency() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    config.nullOutputTargetLatencyMs = 17;
    AudioNullOutput output(clock, log, config);

    assert(output.targetLatencyMs() == 17);
    output.configureTiming(1000, 1, 1);
    assert(output.targetDelaySamples() == 17);
}

static void testPulseOutputReceivesServerAndLatencyWithoutOpening() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    config.pulseServer = "pulse.local";
    config.pulseLatencyMs = 1234;
    config.pulseOutputTargetLatencyMs = 23;
    AudioPulseOutput output(stereo16Format(), clock, log, config, NULL, 0);

    assert(strcmp(output.serverName(), "pulse.local") == 0);
    assert(strcmp(output.serverDisplayName(), "pulse.local") == 0);
    assert(output.pulseLatencyMs() == 1234);
    assert(output.targetLatencyMs() == 23);
    output.configureTiming(1000, stereo16Format().bytesPerSample(), 1);
    assert(output.targetDelaySamples() == 23);
    assert(output.queuedTargetSamples() == 23);

#if WITH_PULSE == 1
    output.pulseUnderflow();
    assert(output.underflowCount() == 1);
    assert(output.queuedTargetSamples() == 73);
#endif
}

static void testPulseUnderflowCountIsInstanceLocal() {
#if WITH_PULSE == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    AudioPulseOutput first(stereo16Format(), clock, log, config, NULL, 0);
    AudioPulseOutput second(stereo16Format(), clock, log, config, NULL, 0);

    assert(first.underflowCount() == 0);
    assert(second.underflowCount() == 0);
    first.pulseUnderflow();
    first.pulseUnderflow();
    assert(first.underflowCount() == 2);
    assert(second.underflowCount() == 0);
#endif
}

static void testMiniAudioOutputReceivesTargetLatencyWithoutOpening() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    config.miniAudioOutputTargetLatencyMs = 31;
    AudioMiniAudioOutput output(stereo16Format(), clock, log, config, NULL, 0);

    assert(!output.isOpen());
    assert(output.targetLatencyMs() == 31);
    output.configureTiming(1000, stereo16Format().bytesPerSample(), 1);
    assert(output.targetDelaySamples() == 31);
    assert(output.queuedTargetSamples() == 31);
#endif
}

static void testMiniAudioDeviceNamesPropagateToRuntimeSnapshots() {
    FakeLogSink log;
    AudioConfig config;
    config.miniAudioPlaybackDeviceName = "Speakers";
    config.miniAudioCaptureDeviceName = "Microphone";

    AudioOutputConfig outputConfig = AudioOutputConfig::fromConfig(config);
    assert(outputConfig.miniAudioPlaybackDeviceName == "Speakers");

    AudioSettings settings = AudioSettings::fromConfig(config, log);
    assert(strcmp(settings.miniAudioCaptureDeviceName, "Microphone") == 0);
}

static void testMiniAudioCallbackDrainRequiresOpenDevice() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    AudioMiniAudioOutput output(stereo16Format(), clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, stereo16Format(), 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);

    assert(!output.isOpen());
    assert(!output.supportsCallbackDrain());
    output.startCallbackDrain(stream, &inputFinished, 4);
    output.notifyCallbackDrain();
    assert(!output.callbackDrainComplete(stream, inputFinished.load()));
    output.stopCallbackDrain();
    assert(!output.callbackDrainComplete(stream, inputFinished.load()));
#endif
}

static void testMiniAudioPcmFormatMapping() {
#if WITH_MINIAUDIO == 1
    int directCopy = 0;

    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(unsigned8MonoFormat(),
               &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatU8);
    assert(directCopy == 1);

    directCopy = 0;
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(signed8MonoFormat(),
               &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 0);

    directCopy = 0;
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(
               unsigned16LeMonoFormat(), &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 0);

    directCopy = 0;
#ifdef WORDS_BIGENDIAN
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(
               formatFor(1000, 1, SF_s16_be), &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 1);
    directCopy = 0;
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(
               formatFor(1000, 1, SF_s16_le), &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 0);
#else
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(
               formatFor(1000, 1, SF_s16_le), &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 1);
    directCopy = 0;
    assert(AudioMiniAudioOutput::testDeviceSampleFormatFor(
               formatFor(1000, 1, SF_s16_be), &directCopy)
        == AudioMiniAudioOutput::MiniAudioDeviceSampleFormatS16);
    assert(directCopy == 0);
#endif
#endif
}

static void testMiniAudioNullBackendNameIsNotARealDevice() {
#if WITH_MINIAUDIO == 1
    assert(AudioMiniAudioOutput::testBackendNameIsNull("Null"));
    assert(!AudioMiniAudioOutput::testBackendNameIsNull("ALSA"));
    assert(!AudioMiniAudioOutput::testBackendNameIsNull("PulseAudio"));
    assert(!AudioMiniAudioOutput::testBackendNameIsNull("Core Audio"));
    assert(!AudioMiniAudioOutput::testBackendNameIsNull(NULL));
#endif
}

static void testMiniAudioPresentationDelayUsesCallbackPeriod() {
#if WITH_MINIAUDIO == 1
    assert(AudioMiniAudioOutput::testPresentationDelaySamples(44100, 250,
        1764, 5) == 1764);
    assert(AudioMiniAudioOutput::testPresentationDelaySamples(44100, 250,
        2400, 4) == 2400);
    assert(AudioMiniAudioOutput::testPresentationDelaySamples(44100, 250,
        0, 0) == 2205);
#endif
}

static void testMiniAudioCallbackDrainDirectCopyCompletesFiniteInput() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = unsigned8MonoFormat();
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    AudioByte pcm[3] = { 0, 128, 255 };
    AudioByte out[5] = { 42, 42, 42, 42, 42 };

    assert(history.appendDecodedPcm((const char*)pcm, 3) == 3);
    output.testStartCallbackDrainWithoutDevice(stream, &inputFinished, 2);
    output.testDrainCallback(out, 5);

    assert(out[0] == 0);
    assert(out[1] == 128);
    assert(out[2] == 255);
    assert(out[3] == 128);
    assert(out[4] == 128);
    assert(stream.submittedEndPosition() == 3);
    assert(stream.queuedForOutputSamples() == 0);
    assert(output.callbackDrainComplete(stream, inputFinished.load()));
    assert(output.underflowCount() == 0);
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioCallbackDrainConvertsSigned8AndPadsSilence() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = signed8MonoFormat();
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    AudioByte pcm[3] = { 0x80, 0x00, 0x7f };
    int16_t out[5];

    for (int i = 0; i < 5; i++)
        out[i] = 1234;

    assert(history.appendDecodedPcm((const char*)pcm, 3) == 3);
    output.testStartCallbackDrainWithoutDevice(stream, &inputFinished, 4);
    output.testDrainCallback(out, 5);

    assert(out[0] == -32768);
    assert(out[1] == 0);
    assert(out[2] == 32512);
    assert(out[3] == 0);
    assert(out[4] == 0);
    assert(stream.submittedEndPosition() == 3);
    assert(output.callbackDrainComplete(stream, inputFinished.load()));
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioCallbackDrainConvertsUnsigned16Le() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = unsigned16LeMonoFormat();
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    AudioByte pcm[6] = { 0x00, 0x00, 0x00, 0x80, 0xff, 0xff };
    int16_t out[3] = { 1234, 1234, 1234 };

    assert(history.appendDecodedPcm((const char*)pcm, 3) == 3);
    output.testStartCallbackDrainWithoutDevice(stream, &inputFinished, 3);
    output.testDrainCallback(out, 3);

    assert(out[0] == -32768);
    assert(out[1] == 0);
    assert(out[2] == 32767);
    assert(stream.submittedEndPosition() == 3);
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioCallbackDrainUpmixesMonoToStereoU8() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = signed8MonoFormat();
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    AudioByte pcm[3] = { 0x80, 0x00, 0x7f };
    AudioByte out[8] = { 42, 42, 42, 42, 42, 42, 42, 42 };

    assert(history.appendDecodedPcm((const char*)pcm, 3) == 3);
    output.testStartCallbackDrainWithoutDeviceFormat(stream, &inputFinished,
        4, AudioMiniAudioOutput::MiniAudioDeviceSampleFormatU8, 2);
    output.testDrainCallback(out, 4);

    assert(out[0] == 0);
    assert(out[1] == 0);
    assert(out[2] == 128);
    assert(out[3] == 128);
    assert(out[4] == 255);
    assert(out[5] == 255);
    assert(out[6] == 128);
    assert(out[7] == 128);
    assert(stream.submittedEndPosition() == 3);
    assert(output.callbackDrainComplete(stream, inputFinished.load()));
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioCallbackDrainDownmixesStereoToMonoF32() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = formatFor(1000, 2, SF_s16_le);
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    AudioByte pcm[8] = {
        0x00, 0x80, 0x00, 0x00,
        0xff, 0x7f, 0x01, 0x80
    };
    float out[3] = { 2.0f, 2.0f, 2.0f };

    assert(history.appendDecodedPcm((const char*)pcm, 2) == 2);
    output.testStartCallbackDrainWithoutDeviceFormat(stream, &inputFinished,
        3, AudioMiniAudioOutput::MiniAudioDeviceSampleFormatF32, 1);
    output.testDrainCallback(out, 3);

    assert(out[0] > -0.501f && out[0] < -0.499f);
    assert(out[1] > -0.001f && out[1] < 0.001f);
    assert(out[2] == 0.0f);
    assert(stream.submittedEndPosition() == 2);
    assert(output.callbackDrainComplete(stream, inputFinished.load()));
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioWavFixtureCallbackDrainCompletesFiniteInput() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    char path[PATH_MAX];
    fixturePath(path, sizeof(path), "sine-50-1600-doubling-4s.wav");
    WavPcmSource source(path, log);

    assert(!source.hasError());

    PcmFormat format = source.format();
    const int chunkSamples = 256;
    const int wantedSamples = 1024;
    int bytesPerSample = format.bytesPerSample();
    char* scratch = new char[pcmBytesForSamples(chunkSamples, bytesPerSample)];
    DecodedAudioHistory history(wantedSamples + chunkSamples, format,
        chunkSamples, log);
    int totalSamples = 0;

    while (totalSamples < wantedSamples) {
        int wanted = wantedSamples - totalSamples;
        if (wanted > chunkSamples)
            wanted = chunkSamples;
        int samples = source.read(scratch,
            pcmBytesForSamples(chunkSamples, bytesPerSample), wanted);
        if (samples <= 0)
            break;
        assert(history.appendDecodedPcm(scratch, samples) == samples);
        totalSamples += samples;
    }
    delete[] scratch;
    assert(totalSamples == wantedSamples);

    AudioOutputConfig config;
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    char* out = new char[pcmBytesForSamples(chunkSamples, bytesPerSample)];

    output.testStartCallbackDrainWithoutDevice(stream, &inputFinished,
        chunkSamples);
    for (int i = 0; i < 8 && !output.callbackDrainComplete(stream,
             inputFinished.load());
         i++) {
        output.testDrainCallback(out, chunkSamples);
    }

    assert(stream.queuedForOutputSamples() == 0);
    assert(output.callbackDrainComplete(stream, inputFinished.load()));
    output.stopCallbackDrain();
    delete[] out;
#endif
}

static void testMiniAudioUnderflowCountIsInstanceLocal() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = signed8MonoFormat();
    AudioMiniAudioOutput first(format, clock, log, config, NULL, 0);
    AudioMiniAudioOutput second(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(0);
    int16_t out[4] = { 1234, 1234, 1234, 1234 };

    first.testStartCallbackDrainWithoutDevice(stream, &inputFinished, 4);
    first.testDrainCallback(out, 4);

    assert(first.underflowCount() == 1);
    assert(second.underflowCount() == 0);
    assert(out[0] == 0);
    assert(out[1] == 0);
    assert(out[2] == 0);
    assert(out[3] == 0);
    first.stopCallbackDrain();
#endif
}

static void testMiniAudioUpdatePollsDiagnosticsWithoutOpeningDevice() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    PcmFormat format = signed8MonoFormat();
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 0);
    DecodedAudioHistory history(16, format, 8, log);
    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(0);
    int16_t out[2] = { 1234, 1234 };

    output.testStartCallbackDrainWithoutDevice(stream, &inputFinished, 2);
    output.testDrainCallback(out, 2);
    assert(output.underflowCount() == 1);
    assert(!output.isOpen());

    output.notifyCallbackDrain();
    assert(!output.isOpen());
    assert(output.underflowCount() == 1);

    output.update();
    assert(!output.isOpen());
    assert(output.underflowCount() == 1);
    output.stopCallbackDrain();
#endif
}

static void testMiniAudioPlaybackConnectedDiagnosticsIncludeDeviceShape() {
#if WITH_MINIAUDIO == 1
    FakeSecondsClock clock;
    CapturingLogSink log;
    AudioOutputConfig config;
    config.miniAudioOutputTargetLatencyMs = 37;
    AudioMiniAudioOutput output(stereo16Format(), clock, log, config, NULL, 0);

    output.testLogConnectedDiagnostics("Core Audio", "Built-in Output",
        48000, 2, AudioMiniAudioOutput::MiniAudioDeviceSampleFormatF32, 0,
        9, 4, 512, 3, 3333);

    assert(log.contains("miniaudio playback connected"));
    assert(log.contains("backend=Core Audio"));
    assert(log.contains("device=`Built-in Output'"));
    assert(log.contains("rate=48000"));
    assert(log.contains("channels=2"));
    assert(log.contains("format="));
    assert(log.contains("direct-copy=0"));
    assert(log.contains("target-latency-ms=37"));
    assert(log.contains("requested-period-ms=9"));
    assert(log.contains("requested-periods=4"));
    assert(log.contains("internal-period-frames=512"));
    assert(log.contains("internal-periods=3"));
    assert(log.contains("presentation-delay-samples=3333"));
#endif
}

static void testDspOutputReceivesTargetLatency() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioSettings settings;
    settings.pcmFormat = stereo16Format();
    settings.soundDSPMethod = 0;
    settings.dspFragments = 8;
    settings.dspFragmentSize = 9;
    strncpy(settings.dspDevicePath, "/tmp/cthughanix-no-such-dsp", PATH_MAX);
    settings.dspDevicePath[PATH_MAX - 1] = '\0';

    AudioOutputConfig config;
    config.dspOutputTargetLatencyMs = 29;
    AudioDSPOutput output(settings, config, 256, clock, log);

    assert(!output.isOpen());
    assert(output.targetLatencyMs() == 29);
    output.configureTiming(1000, stereo16Format().bytesPerSample(), 1);
    assert(output.targetDelaySamples() == 29);
}

int main() {
    testNullOutputReceivesTargetLatency();
    testPulseOutputReceivesServerAndLatencyWithoutOpening();
    testPulseUnderflowCountIsInstanceLocal();
    testMiniAudioOutputReceivesTargetLatencyWithoutOpening();
    testMiniAudioDeviceNamesPropagateToRuntimeSnapshots();
    testMiniAudioCallbackDrainRequiresOpenDevice();
    testMiniAudioPcmFormatMapping();
    testMiniAudioNullBackendNameIsNotARealDevice();
    testMiniAudioPresentationDelayUsesCallbackPeriod();
    testMiniAudioCallbackDrainDirectCopyCompletesFiniteInput();
    testMiniAudioCallbackDrainConvertsSigned8AndPadsSilence();
    testMiniAudioCallbackDrainConvertsUnsigned16Le();
    testMiniAudioCallbackDrainUpmixesMonoToStereoU8();
    testMiniAudioCallbackDrainDownmixesStereoToMonoF32();
    testMiniAudioWavFixtureCallbackDrainCompletesFiniteInput();
    testMiniAudioUnderflowCountIsInstanceLocal();
    testMiniAudioUpdatePollsDiagnosticsWithoutOpeningDevice();
    testMiniAudioPlaybackConnectedDiagnosticsIncludeDeviceShape();
    testDspOutputReceivesTargetLatency();
    return 0;
}
