/** @file
 * Unit coverage for runtime audio controls over owned processing state.
 */

#include "RuntimeAudioControls.h"
#include "AudioProcessing.h"
#include "AudioProcessor.h"
#include "Mixer.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

Option::~Option() { }

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeRandomSource : public RandomSource {
public:
    int value;
    int calls;

    FakeRandomSource()
        : value(0)
        , calls(0) { }

    virtual int uniformInt(int exclusiveMax) {
        calls++;
        if (exclusiveMax <= 1)
            return 0;
        return value % exclusiveMax;
    }
};

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const { return 0; }
};

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

class FakeMixerDevice : public MixerDevice {
public:
    std::vector<MixerChannel> discoveredChannels;
    int setVolumeCalls;
    int setVolumeEncodedVolume;

    FakeMixerDevice()
        : discoveredChannels()
        , setVolumeCalls(0)
        , setVolumeEncodedVolume(0) { }

    virtual int initialize(const std::string&,
        const std::vector<MixerInitialVolume>&,
        std::vector<MixerChannel>& channels, LogSink& log) {
        (void)log;
        channels = discoveredChannels;
        return 0;
    }

    virtual int setVolume(const std::string&, const MixerChannel&,
        int encodedVolume, int& active, LogSink& log) {
        (void)log;
        setVolumeCalls++;
        setVolumeEncodedVolume = encodedVolume;
        active = encodedVolume > 0;
        return 0;
    }
};

static void testDirectCommandsChangeOnlyAudioProcessing() {
    AudioProcessor processor;
    FakeRandomSource randomSource;
    FakeLogSink log;
    AudioProcessingState state(randomSource);
    AudioProcessingSelector selector(state, processor, log);
    DefaultRuntimeAudioControls controls(selector);

    controls.changeSoundProcessingBy(3);
    assert(strcmp(selector.text(), "FFT") == 0);

    controls.changeSoundProcessingTo("filter1");
    assert(strcmp(selector.text(), "Filter1") == 0);
}

static void testGenericOptionRoutingClaimsOnlyAudioProcessing() {
    AudioProcessor processor;
    FakeRandomSource randomSource;
    FakeLogSink log;
    AudioProcessingState state(randomSource);
    AudioProcessingSelector selector(state, processor, log);
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

static void testGenericOptionRoutingClaimsMixerOptionsAsUiChanges() {
    AudioProcessor processor;
    FakeRandomSource randomSource;
    FakeLogSink log;
    AudioProcessingState state(randomSource);
    AudioProcessingSelector selector(state, processor, log);
    FakeMixerDevice device;
    device.discoveredChannels.push_back(
        MixerChannel("line", 1, 2 + 256 * 3, 1));
    MixerSession mixerSession(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(mixerSession.initialize() == 0);
    MixerControls mixerControls(mixerSession, log);
    DefaultRuntimeAudioControls controls(selector, &mixerControls);

    RuntimeChangeSet byChanges;
    int handled = controls.changeAudioOptionBy(
        *mixerControls.optionAt(0), 4, byChanges);
    assert(handled == 1);
    assert(byChanges.uiChanged == 1);
    assert(byChanges.audioProcessingChanged == 0);
    assert(device.setVolumeCalls == 1);
    assert(device.setVolumeEncodedVolume == 6 + 256 * 7);

    RuntimeChangeSet toChanges;
    handled = controls.changeAudioOptionTo(
        *mixerControls.optionAt(0), "11", toChanges);
    assert(handled == 1);
    assert(toChanges.uiChanged == 1);
    assert(toChanges.audioProcessingChanged == 0);
    assert(device.setVolumeCalls == 2);
    assert(device.setVolumeEncodedVolume == 11 + 256 * 11);
}

static void testUnknownAudioProcessingSelectionUsesInjectedRandomSource() {
    AudioProcessor processor;
    FakeRandomSource randomSource;
    FakeLogSink log;
    AudioProcessingState state(randomSource);
    AudioProcessingSelector selector(state, processor, log);

    randomSource.value = 2;
    selector.changeTo("");
    assert(strcmp(selector.text(), "Filter2") == 0);
    assert(randomSource.calls == 1);

    randomSource.value = 3;
    selector.changeTo("not-a-mode");
    assert(strcmp(selector.text(), "FFT") == 0);
    assert(randomSource.calls == 2);
}

int main() {
    testDirectCommandsChangeOnlyAudioProcessing();
    testGenericOptionRoutingClaimsOnlyAudioProcessing();
    testGenericOptionRoutingClaimsMixerOptionsAsUiChanges();
    testUnknownAudioProcessingSelectionUsesInjectedRandomSource();
    return 0;
}
