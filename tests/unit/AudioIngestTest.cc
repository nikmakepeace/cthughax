/** @file
 * Unit coverage for AudioIngest acquisition pacing and completion.
 */

#include "AudioIngest.h"

#include <assert.h>
#include <memory>
#include <stdarg.h>
#include <string.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

static double fakeSystemTime = 0.0;
double getTime() { return fakeSystemTime; }

static PcmFormat currentFormat;
static int inputLoopEnabled = 0;

const PcmFormat& audioPcmFormat() { return currentFormat; }
AudioInputMode audioInputModeValue() { return AIM_File; }
int audioInputLoopEnabled() { return inputLoopEnabled; }
void audioSetInputLoopEnabled(int enabled) { inputLoopEnabled = enabled ? 1 : 0; }
int audioSampleRateHz() { return currentFormat.sampleRate; }
int audioChannels() { return currentFormat.channels; }
int audioSampleFormat() { return currentFormat.sampleFormat; }
const char* audioSampleFormatText() { return "signed-8"; }
const char* audioSampleFormatText(int) { return "signed-8"; }
void audioSetPcmFormat(const PcmFormat& format) { currentFormat = format; }
void audioSetSampleRateHz(int sampleRateHz) { currentFormat.sampleRate = sampleRateHz; }
void audioSetChannels(int channels) { currentFormat.channels = channels; }
void audioSetSampleFormat(int sampleFormat) { currentFormat.sampleFormat = sampleFormat; }
int audioBytesPerSample() { return currentFormat.bytesPerSample(); }

class FakeClock : public AudioIngestClock {
public:
    double value;

    FakeClock()
        : value(0.0) { }

    virtual double nowSeconds() const { return value; }
};

class VectorPcmSource : public PcmSource {
    std::vector<char> data;
    int cursorSamples;

public:
    VectorPcmSource(int samples) : data(samples), cursorSamples(0) {
        pcmFormat.sampleRate = 1000;
        pcmFormat.channels = 1;
        pcmFormat.sampleFormat = SF_s8;
        for (int i = 0; i < samples; i++)
            data[i] = char(i % 100);
    }

    virtual int read(char* dst, int rawSize, int samplesRequested) {
        int remaining = int(data.size()) - cursorSamples;
        int samples = samplesRequested;
        if (samples > remaining)
            samples = remaining;
        if (samples > rawSize)
            samples = rawSize;
        if (samples <= 0)
            return 0;

        memcpy(dst, data.data() + cursorSamples, samples);
        cursorSamples += samples;
        return samples;
    }

    virtual int canFinish() const { return 1; }
};

static AudioIngest* makeIngest(FakeClock& clock, int samples) {
    return new AudioIngest(new AudioInput(new VectorPcmSource(samples)), NULL, 256,
        clock, 1, 0);
}

static void testTickBuildsFramesFromSilentPassthroughMode() {
    FakeClock clock;
    std::unique_ptr<AudioIngest> ingest(makeIngest(clock, 4096));

    assert(ingest->start(0) == 0);
    ingest->tick();
    assert(ingest->currentFrame().samples > 0);
    assert(ingest->currentFrame().centerSample == 0);

    clock.value = 1.0;
    ingest->tick();
    assert(ingest->currentFrame().centerSample == 1000);
    assert(ingest->decodedSamplePosition() >= 1000);
}

static void testFiniteSilentInputCompletesAtVisualEof() {
    FakeClock clock;
    std::unique_ptr<AudioIngest> ingest(makeIngest(clock, 1500));

    assert(ingest->start(0) == 0);
    ingest->tick();
    assert(!ingest->complete());

    clock.value = 2.0;
    ingest->tick();
    assert(ingest->complete());
}

int main() {
    currentFormat.sampleRate = 1000;
    currentFormat.channels = 1;
    currentFormat.sampleFormat = SF_s8;

    testTickBuildsFramesFromSilentPassthroughMode();
    testFiniteSilentInputCompletesAtVisualEof();
    return 0;
}
