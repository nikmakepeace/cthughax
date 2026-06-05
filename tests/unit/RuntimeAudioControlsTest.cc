/** @file
 * Unit coverage for runtime audio controls over owned processing state.
 */

#include "RuntimeAudioControls.h"
#include "AudioProcessing.h"
#include "AudioProcessor.h"

#include <assert.h>
#include <string.h>

Option::~Option() { }

class RecordingOption : public Option {
public:
    int byCalls;
    int toCalls;

    RecordingOption()
        : Option("recording")
        , byCalls(0)
        , toCalls(0) { }

    virtual void change(int) {
        byCalls++;
    }

    virtual void change(const char*) {
        toCalls++;
    }

    virtual const char* text() const {
        return "recording";
    }
};

static void testDirectCommandsChangeOnlyAudioProcessing() {
    AudioProcessor processor;
    AudioProcessingState state;
    AudioProcessingSelector selector(state, processor);
    DefaultRuntimeAudioControls controls(selector);

    controls.changeSoundProcessingBy(3);
    assert(strcmp(selector.text(), "FFT") == 0);

    controls.changeSoundProcessingTo("filter1");
    assert(strcmp(selector.text(), "Filter1") == 0);
}

static void testGenericOptionRoutingClaimsOnlyAudioProcessing() {
    AudioProcessor processor;
    AudioProcessingState state;
    AudioProcessingSelector selector(state, processor);
    DefaultRuntimeAudioControls controls(selector);

    RuntimeChangeSet byChanges;
    int handled = controls.changeAudioOptionBy(selector.option(), 2, byChanges);
    assert(handled == 1);
    assert(byChanges.audioProcessingChanged == 1);
    assert(strcmp(selector.text(), "Filter2") == 0);

    RuntimeChangeSet toChanges;
    handled = controls.changeAudioOptionTo(selector.option(), "filter1", toChanges);
    assert(handled == 1);
    assert(toChanges.audioProcessingChanged == 1);
    assert(strcmp(selector.text(), "Filter1") == 0);

    RecordingOption unrelated;
    RuntimeChangeSet unrelatedChanges;
    handled = controls.changeAudioOptionBy(unrelated, 9, unrelatedChanges);
    assert(handled == 0);
    assert(!unrelatedChanges.any());
    assert(unrelated.byCalls == 0);

    handled = controls.changeAudioOptionTo(unrelated, "anything", unrelatedChanges);
    assert(handled == 0);
    assert(!unrelatedChanges.any());
    assert(unrelated.toCalls == 0);
}

int main() {
    testDirectCommandsChangeOnlyAudioProcessing();
    testGenericOptionRoutingClaimsOnlyAudioProcessing();
    return 0;
}
