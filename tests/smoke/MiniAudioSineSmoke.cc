/** @file
 * Opt-in miniaudio generated-PCM playback smoke test.
 */

#include "Audio.h"
#include "ProcessServices.h"
#include "config.h"

#include <atomic>
#include <chrono>
#include <math.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <thread>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
const char* audioSampleFormatText(int) { return "signed-16"; }

class SmokeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

class SmokeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

static PcmFormat smokePlaybackFormat() {
    PcmFormat format;
    format.sampleRate = 48000;
    format.channels = 2;
    format.sampleFormat = SF_s16_le;
    return format;
}

static void writeSinePcm(AudioByte* pcm, int frames, const PcmFormat& format) {
    const double pi = 3.14159265358979323846;
    const double frequency = 440.0;
    const double amplitude = 0.20 * 32767.0;

    for (int frame = 0; frame < frames; frame++) {
        double phase = (2.0 * pi * frequency * frame) / format.sampleRate;
        int16_t value = (int16_t)(sin(phase) * amplitude);
        for (int channel = 0; channel < format.channels; channel++) {
            int offset = (frame * format.channels + channel) * 2;
            pcm[offset] = (AudioByte)(value & 0xff);
            pcm[offset + 1] = (AudioByte)((value >> 8) & 0xff);
        }
    }
}

int main() {
#if WITH_MINIAUDIO == 1
    SmokeSecondsClock clock;
    SmokeLogSink log;
    AudioOutputConfig config;
    config.miniAudioOutputTargetLatencyMs = 100;
    PcmFormat format = smokePlaybackFormat();

    AudioMiniAudioOutput output(format, clock, log, config, NULL, 1);
    if (!output.isOpen()) {
        if (strcmp(output.backendName(), "Null") == 0) {
            fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
            return 4;
        }
        fprintf(stderr, "miniaudio default playback device did not open\n");
        return 2;
    }

    if (!output.supportsCallbackDrain()) {
        fprintf(stderr, "miniaudio output opened without callback drain support\n");
        return 3;
    }

    if (strcmp(output.backendName(), "Null") == 0) {
        fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
        return 4;
    }

    const int frames = format.sampleRate / 5;
    const int bytes = pcmBytesForSamples(frames, format.bytesPerSample());
    AudioByte* pcm = new AudioByte[bytes];
    writeSinePcm(pcm, frames, format);

    DecodedAudioHistory history(frames + 4096, format, 4096, log);
    if (history.appendDecodedPcm((const char*)pcm, frames) != frames) {
        delete[] pcm;
        fprintf(stderr, "failed to append generated sine PCM\n");
        return 5;
    }
    delete[] pcm;

    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    output.configureTiming(format.sampleRate, format.bytesPerSample(), 1024);
    output.startCallbackDrain(stream, &inputFinished, 1024);

    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::seconds(3);
    while (!output.callbackDrainComplete(stream, inputFinished.load())
        && (std::chrono::steady_clock::now() < deadline)) {
        output.notifyCallbackDrain();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    output.stopCallbackDrain();

    if (stream.queuedForOutputSamples() != 0) {
        fprintf(stderr,
            "miniaudio sine playback did not drain queued PCM, remaining samples=%d\n",
            stream.queuedForOutputSamples());
        return 6;
    }

    return 0;
#else
    fprintf(stderr, "miniaudio support is not compiled in\n");
    return 77;
#endif
}
