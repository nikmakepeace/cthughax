/** @file
 * Unit coverage for AudioIngest acquisition pacing and completion.
 */

#include "AudioIngest.h"
#include "ProcessServices.h"

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

const char* audioSampleFormatText(int) { return "signed-8"; }

class FakeClock : public SecondsClock {
public:
    double value;

    FakeClock()
        : value(0.0) { }

    virtual double nowSeconds() const { return value; }
};

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

class VectorPcmSource : public PcmSource {
    std::vector<char> data;
    int cursorSamples;

public:
    VectorPcmSource(int samples, LogSink& log)
        : PcmSource(log)
        , data(samples)
        , cursorSamples(0) {
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

static AudioIngest* makeIngest(FakeClock& clock, FakeLogSink& log,
    int samples) {
    return new AudioIngest(new AudioInput(new VectorPcmSource(samples, log), log),
        NULL, 256,
        clock, log, 1, 0);
}

static void testTickBuildsFramesFromSilentPassthroughMode() {
    FakeClock clock;
    FakeLogSink log;
    std::unique_ptr<AudioIngest> ingest(makeIngest(clock, log, 4096));

    assert(ingest->start() == 0);
    assert(ingest->format().sampleRate == 1000);
    assert(ingest->format().channels == 1);
    assert(ingest->format().sampleFormat == SF_s8);
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
    FakeLogSink log;
    std::unique_ptr<AudioIngest> ingest(makeIngest(clock, log, 1500));

    assert(ingest->start() == 0);
    ingest->tick();
    assert(!ingest->complete());

    clock.value = 2.0;
    ingest->tick();
    assert(ingest->complete());
}

int main() {
    testTickBuildsFramesFromSilentPassthroughMode();
    testFiniteSilentInputCompletesAtVisualEof();
    return 0;
}
