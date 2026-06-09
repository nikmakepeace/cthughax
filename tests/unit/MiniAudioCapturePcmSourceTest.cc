/** @file
 * Hardware-free coverage for the miniaudio capture PCM source.
 */

#include "Audio.h"
#include "AudioSettings.h"
#include "ProcessServices.h"
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const { return 0; }
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

    virtual int enabled(int) const { return 1; }

    int contains(const char* token) const {
        return strstr(text, token) != NULL;
    }
};

static int nativeS16Format() {
#ifdef WORDS_BIGENDIAN
    return SF_s16_be;
#else
    return SF_s16_le;
#endif
}

static AudioSettings captureSettings(int sampleRate, int channels,
    int sampleFormat) {
    AudioSettings settings;
    settings.audioInputMode = AIM_DSPIn;
    settings.pcmFormat.sampleRate = sampleRate;
    settings.pcmFormat.channels = channels;
    settings.pcmFormat.sampleFormat = sampleFormat;
    return settings;
}

static void testUnsupportedCaptureFormatNegotiatesNativeS16() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(0, 3, SF_u16_le);
    MiniAudioCapturePcmSource source(settings, log, 0);

    assert(!source.hasError());
    assert(!source.isOpen());
    assert(source.format().sampleRate == 44100);
    assert(source.format().channels == 2);
    assert(source.format().sampleFormat == nativeS16Format());
}

static void testCaptureRingReadReturnsBufferedFrames() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);
    unsigned char input[] = {
        1, 2,
        3, 4,
        5, 6,
        7, 8
    };
    unsigned char output[8];

    source.testInitializeRingWithoutDevice(8);
    source.testWriteCapturedFrames(input, 4);
    assert(source.testAvailableReadFrames() == 4);

    memset(output, 0, sizeof(output));
    assert(source.read((char*)output, 4, 2) == 2);
    assert(memcmp(output, input, 4) == 0);
    assert(source.testAvailableReadFrames() == 2);

    memset(output, 0, sizeof(output));
    assert(source.read((char*)output, sizeof(output), 4) == 2);
    assert(memcmp(output, input + 4, 4) == 0);
    assert(source.testAvailableReadFrames() == 0);
}

static void testCaptureRingOverrunIsCounted() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);
    unsigned char input[] = {
        1, 2,
        3, 4,
        5, 6,
        7, 8
    };

    source.testInitializeRingWithoutDevice(2);
    source.testWriteCapturedFrames(input, 4);

    assert(source.testAvailableReadFrames() == 2);
    assert(source.overrunCount() == 1);
}

static void testCaptureRawBufferSizeUsesNegotiatedFormat() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);

    assert(source.rawBufferSize(1, 10) == 20);
}

static void testCaptureRingCanUseAcceptedMonoS16Format() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);
    int16_t input[] = { -1234, 2345, 0 };
    int16_t output[3] = { 111, 111, 111 };

    source.testInitializeRingWithoutDeviceFormat(32000, 1, nativeS16Format(),
        8);
    assert(source.format().sampleRate == 32000);
    assert(source.format().channels == 1);
    assert(source.format().sampleFormat == nativeS16Format());
    assert(source.rawBufferSize(1, 3) == 6);

    source.testWriteCapturedFrames(input, 3);
    assert(source.read((char*)output, sizeof(output), 3) == 3);
    assert(memcmp(output, input, sizeof(input)) == 0);
}

static void testCaptureNullBackendNameIsNotARealDevice() {
    assert(MiniAudioCapturePcmSource::testBackendNameIsNull("Null"));
    assert(!MiniAudioCapturePcmSource::testBackendNameIsNull("ALSA"));
    assert(!MiniAudioCapturePcmSource::testBackendNameIsNull("PulseAudio"));
    assert(!MiniAudioCapturePcmSource::testBackendNameIsNull("Core Audio"));
    assert(!MiniAudioCapturePcmSource::testBackendNameIsNull(NULL));
}

static void testCaptureReinitializationUsesRequestedFormat() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);

    source.testInitializeRingWithoutDeviceFormat(32000, 1, nativeS16Format(),
        8);
    assert(source.format().sampleRate == 32000);
    assert(source.format().channels == 1);
    assert(source.format().sampleFormat == nativeS16Format());

    source.testInitializeRingWithoutDevice(8);
    assert(source.format().sampleRate == 48000);
    assert(source.format().channels == 2);
    assert(source.format().sampleFormat == SF_u8);
}

static void testNoDeviceRingInitializationClearsPreviousError() {
    FakeLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);

    source.testSetError(1);
    assert(source.hasError());

    source.testInitializeRingWithoutDevice(8);
    assert(!source.hasError());
}

static void testCaptureConnectedDiagnosticsIncludeDeviceShape() {
    CapturingLogSink log;
    AudioSettings settings = captureSettings(48000, 2, SF_u8);
    MiniAudioCapturePcmSource source(settings, log, 0);

    source.testLogConnectedDiagnostics("ALSA", "USB Capture", 32000, 1,
        nativeS16Format(), 4096);

    assert(log.contains("miniaudio capture connected"));
    assert(log.contains("backend=ALSA"));
    assert(log.contains("device=`USB Capture'"));
    assert(log.contains("rate=32000"));
    assert(log.contains("channels=1"));
    assert(log.contains("format="));
    assert(log.contains("ring-frames=4096"));
}

int main() {
    testUnsupportedCaptureFormatNegotiatesNativeS16();
    testCaptureRingReadReturnsBufferedFrames();
    testCaptureRingOverrunIsCounted();
    testCaptureRawBufferSizeUsesNegotiatedFormat();
    testCaptureRingCanUseAcceptedMonoS16Format();
    testCaptureNullBackendNameIsNotARealDevice();
    testCaptureReinitializationUsesRequestedFormat();
    testNoDeviceRingInitializationClearsPreviousError();
    testCaptureConnectedDiagnosticsIncludeDeviceShape();
    return 0;
}
