/** @file
 * Unit coverage for audio passthrough cursor ownership.
 */

#include "AudioPassthrough.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
double getTime() { return 0.0; }

int audioSampleRateHz() { return 1000; }
int audioChannels() { return 1; }
int audioSampleFormat() { return SF_s8; }
const char* audioSampleFormatText() { return "signed-8"; }
const char* audioSampleFormatText(int) { return "signed-8"; }
int audioBytesPerSample() { return 1; }

class RecordingOutput : public AudioOutput {
    int realtimeValue;

protected:
    virtual int defaultTargetLatencyMs() const { return 0; }

public:
    int bytesWritten;
    int maxBytesPerWrite;

    RecordingOutput(int realtime = 0, int maxBytes = 0)
        : realtimeValue(realtime)
        , bytesWritten(0)
        , maxBytesPerWrite(maxBytes) { }

    virtual int write(const void*, int size) {
        int accepted = size;
        if ((maxBytesPerWrite > 0) && (accepted > maxBytesPerWrite))
            accepted = maxBytesPerWrite;
        bytesWritten += accepted;
        return accepted;
    }

    virtual int isOpen() const { return 1; }
    virtual int isRealtime() const { return realtimeValue; }
};

static void testPassthroughAdvancesOnlyItsCursor() {
    DecodedAudioHistory history(16, 1, 8);
    char pcm[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    std::atomic<int> inputFinished(0);
    AudioPassthrough passthrough(new RecordingOutput(), history, inputFinished);

    assert(history.appendDecodedPcm(pcm, 8) == 8);
    assert(passthrough.start(1000, 1, 4, 0) == 0);
    assert(passthrough.serviceOnce() == 1);

    assert(history.decodedEndPosition() == 8);
    assert(passthrough.submittedSamplePosition() == 4);
    assert(passthrough.queuedSamples() == 4);

    inputFinished.store(1);
    assert(passthrough.serviceOnce() == 1);
    assert(passthrough.complete());
}

static void testPassthroughResyncsWhenHistoryMovedOn() {
    DecodedAudioHistory history(8, 1, 3);
    char first[5] = { 0, 1, 2, 3, 4 };
    char second[5] = { 5, 6, 7, 8, 9 };
    std::atomic<int> inputFinished(0);
    AudioPassthrough passthrough(new RecordingOutput(0, 1), history, inputFinished);

    assert(history.appendDecodedPcm(first, 5) == 5);
    assert(history.appendDecodedPcm(second, 5) == 5);
    assert(history.oldestAvailablePosition() == 7);
    assert(passthrough.start(1000, 1, 1, 0) == 0);
    assert(passthrough.serviceOnce() == 1);

    assert(passthrough.submittedSamplePosition() == 8);
    assert(history.decodedEndPosition() == 10);
}

static void testPresentationClockSubtractsTargetDelay() {
    DecodedAudioHistory history(16, 1, 8);
    char pcm[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    std::atomic<int> inputFinished(0);
    AudioPassthrough passthrough(new RecordingOutput(1), history, inputFinished);

    assert(history.appendDecodedPcm(pcm, 8) == 8);
    assert(passthrough.start(1000, 1, 4, 0) == 0);
    assert(passthrough.providesPresentationClock());
    assert(passthrough.serviceOnce() == 1);

    assert(passthrough.submittedSamplePosition() == 4);
    assert(passthrough.presentationSamplePosition() == 0);
}

int main() {
    testPassthroughAdvancesOnlyItsCursor();
    testPassthroughResyncsWhenHistoryMovedOn();
    testPresentationClockSubtractsTargetDelay();
    return 0;
}
