/** @file
 * Unit coverage for mixer Option adapters.
 */

#include "Mixer.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

Option::~Option() { }

class RecordingLogSink : public LogSink {
public:
    int writes;

    RecordingLogSink()
        : writes(0) { }

    virtual int enabled(int) const { return 1; }

protected:
    virtual void write(int, const char*, int, const char*, va_list) {
        writes++;
    }
};

class FakeMixerDevice : public MixerDevice {
public:
    std::vector<MixerChannel> discoveredChannels;
    int setVolumeCalls;
    int setVolumeEncodedVolume;
    int returnedActive;

    FakeMixerDevice()
        : discoveredChannels()
        , setVolumeCalls(0)
        , setVolumeEncodedVolume(0)
        , returnedActive(1) { }

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
        active = returnedActive;
        return 0;
    }
};

class RecordingOption : public Option {
public:
    RecordingOption()
        : Option("recording") { }

    virtual void change(int) { }
    virtual void change(const char*) { }
    virtual const char* text() const { return "recording"; }
};

static void testMixerControlsBuildOptionsFromSessionChannels() {
    RecordingLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(
        MixerChannel("line", 1, 10 + 256 * 20, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(session.initialize() == 0);

    MixerControls controls(session, log);

    assert(controls.optionCount() == 1);
    assert(controls.optionAt(0) != NULL);
    assert(controls.optionAt(1) == NULL);
    assert(strstr(controls.optionAt(0)->text(), "10:20") != NULL);
    assert(strchr(controls.optionAt(0)->text(), '*') != NULL);
}

static void testMixerOptionTextStorageIsPerOption() {
    RecordingLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(
        MixerChannel("line", 1, 10 + 256 * 20, 1));
    device.discoveredChannels.push_back(
        MixerChannel("mic", 2, 30 + 256 * 40, 0));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(session.initialize() == 0);

    MixerControls controls(session, log);
    const char* lineText = controls.optionAt(0)->text();
    const char* micText = controls.optionAt(1)->text();

    assert(lineText != micText);
    assert(strstr(lineText, "10:20") != NULL);
    assert(strchr(lineText, '*') != NULL);
    assert(strstr(micText, "30:40") != NULL);
    assert(strchr(micText, '*') == NULL);
}

static void testMixerControlsMutateOwnedSession() {
    RecordingLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(
        MixerChannel("mic", 2, 3 + 256 * 4, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(session.initialize() == 0);
    MixerControls controls(session, log);
    Option* option = controls.optionAt(0);

    assert(controls.changeOptionBy(*option, 5) == 1);
    assert(device.setVolumeCalls == 1);
    assert(device.setVolumeEncodedVolume == 8 + 256 * 9);
    assert(session.channels()[0].encodedVolume == 8 + 256 * 9);
    assert(strstr(option->text(), "  8:9") != NULL);

    assert(controls.changeOptionTo(*option, "12") == 1);
    assert(device.setVolumeCalls == 2);
    assert(device.setVolumeEncodedVolume == 12 + 256 * 12);
    assert(session.channels()[0].encodedVolume == 12 + 256 * 12);
    assert(strstr(option->text(), "12:12") != NULL);
}

static void testMixerControlsIgnoreUnrelatedOptions() {
    RecordingLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("pcm", 3, 1, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(session.initialize() == 0);
    MixerControls controls(session, log);
    RecordingOption unrelated;

    assert(controls.changeOptionBy(unrelated, 1) == 0);
    assert(controls.changeOptionTo(unrelated, "12") == 0);
    assert(device.setVolumeCalls == 0);
}

static void testInvalidMixerOptionValueUsesInjectedLogSink() {
    RecordingLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("line", 1, 10, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());
    assert(session.initialize() == 0);
    MixerControls controls(session, log);

    assert(controls.changeOptionTo(*controls.optionAt(0), "bad") == 1);
    assert(device.setVolumeCalls == 0);
    assert(log.writes == 1);
}

int main() {
    testMixerControlsBuildOptionsFromSessionChannels();
    testMixerOptionTextStorageIsPerOption();
    testMixerControlsMutateOwnedSession();
    testMixerControlsIgnoreUnrelatedOptions();
    testInvalidMixerOptionValueUsesInjectedLogSink();
    return 0;
}
