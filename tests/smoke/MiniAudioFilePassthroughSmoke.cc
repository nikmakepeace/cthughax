/** @file
 * Opt-in miniaudio WAV fixture playback smoke test.
 */

#include "Audio.h"
#include "ProcessServices.h"
#include "config.h"

#include <atomic>
#include <chrono>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <thread>

#ifndef CTH_AUDIO_FIXTURE_DIR
#error CTH_AUDIO_FIXTURE_DIR must point at tests/fixtures/audio
#endif

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

static void fixturePath(char* path, size_t size, const char* fileName) {
    snprintf(path, size, "%s/%s", CTH_AUDIO_FIXTURE_DIR, fileName);
    path[size - 1] = '\0';
}

int main() {
#if WITH_MINIAUDIO == 1
    SmokeSecondsClock clock;
    SmokeLogSink log;
    char path[PATH_MAX];
    fixturePath(path, sizeof(path), "sine-50-1600-doubling-4s.wav");

    WavPcmSource source(path, log);
    if (source.hasError()) {
        fprintf(stderr, "failed to open WAV fixture `%s'\n", path);
        return 2;
    }

    PcmFormat format = source.format();
    AudioOutputConfig config;
    config.miniAudioOutputTargetLatencyMs = 100;
    AudioMiniAudioOutput output(format, clock, log, config, NULL, 1);
    if (!output.isOpen()) {
        if (strcmp(output.backendName(), "Null") == 0) {
            fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
            return 5;
        }
        fprintf(stderr, "miniaudio default playback device did not open\n");
        return 3;
    }

    if (!output.supportsCallbackDrain()) {
        fprintf(stderr, "miniaudio output opened without callback drain support\n");
        return 4;
    }

    if (strcmp(output.backendName(), "Null") == 0) {
        fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
        return 5;
    }

    const int chunkSamples = 1024;
    const int samplesWanted = format.sampleRate / 5;
    const int bytesPerSample = format.bytesPerSample();
    char* scratch = new char[pcmBytesForSamples(chunkSamples, bytesPerSample)];
    DecodedAudioHistory history(samplesWanted + chunkSamples, format,
        chunkSamples, log);
    int totalSamples = 0;

    while (totalSamples < samplesWanted) {
        int wanted = samplesWanted - totalSamples;
        if (wanted > chunkSamples)
            wanted = chunkSamples;
        int samples = source.read(scratch,
            pcmBytesForSamples(chunkSamples, bytesPerSample), wanted);
        if (samples <= 0)
            break;
        if (history.appendDecodedPcm(scratch, samples) != samples) {
            delete[] scratch;
            fprintf(stderr, "failed to append WAV fixture PCM\n");
            return 6;
        }
        totalSamples += samples;
    }
    delete[] scratch;

    if (totalSamples <= 0) {
        fprintf(stderr, "WAV fixture produced no PCM samples\n");
        return 7;
    }

    AudioOutputStream stream(history);
    std::atomic<int> inputFinished(1);
    output.configureTiming(format.sampleRate, format.bytesPerSample(),
        chunkSamples);
    output.startCallbackDrain(stream, &inputFinished, chunkSamples);

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
            "miniaudio WAV fixture playback did not drain queued PCM, remaining samples=%d\n",
            stream.queuedForOutputSamples());
        return 8;
    }

    return 0;
#else
    fprintf(stderr, "miniaudio support is not compiled in\n");
    return 77;
#endif
}
