/** @file
 * Unit coverage for owned OSS mixer session state.
 */

#include "Mixer.h"
#include "Configuration.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

class NullLogSink : public LogSink {
public:
    virtual int enabled(int) const { return 0; }

protected:
    virtual void write(int, const char*, int, const char*, va_list) { }
};

class FakeMixerDevice : public MixerDevice {
public:
    int initializeCalls;
    int initializeResult;
    std::string initializedPath;
    std::vector<MixerInitialVolume> initializedVolumes;
    std::vector<MixerChannel> discoveredChannels;
    int setVolumeCalls;
    int setVolumeResult;
    std::string setVolumePath;
    int setVolumeDeviceId;
    int setVolumeEncodedVolume;
    int returnedActive;

    FakeMixerDevice()
        : initializeCalls(0)
        , initializeResult(0)
        , initializedPath()
        , initializedVolumes()
        , discoveredChannels()
        , setVolumeCalls(0)
        , setVolumeResult(0)
        , setVolumePath()
        , setVolumeDeviceId(-1)
        , setVolumeEncodedVolume(-1)
        , returnedActive(1) { }

    virtual int initialize(const std::string& path,
        const std::vector<MixerInitialVolume>& initialVolumes,
        std::vector<MixerChannel>& channels, LogSink& log) {
        (void)log;
        initializeCalls++;
        initializedPath = path;
        initializedVolumes = initialVolumes;
        channels = discoveredChannels;
        return initializeResult;
    }

    virtual int setVolume(const std::string& path, const MixerChannel& channel,
        int encodedVolume, int& active, LogSink& log) {
        (void)log;
        setVolumeCalls++;
        setVolumePath = path;
        setVolumeDeviceId = channel.deviceId;
        setVolumeEncodedVolume = encodedVolume;
        active = returnedActive;
        return setVolumeResult;
    }
};

static void testStartupConfigIsCopiedIntoSession() {
    AudioConfig config;
    config.mixerDevicePath = "/tmp/test-mixer";
    AudioMixerInitialVolumeConfig line;
    line.name = "line";
    line.volume = 12 + 256 * 34;
    config.mixerInitialVolumes.push_back(line);

    NullLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("line", 2, line.volume, 1));
    MixerSession session(device, log, config);

    assert(session.path() == "/tmp/test-mixer");
    assert(session.initialVolumes().size() == 1);
    assert(session.initialVolumes()[0].name == "line");
    assert(session.initialVolumes()[0].encodedVolume == line.volume);
    assert(session.initialize() == 0);
    assert(device.initializeCalls == 1);
    assert(device.initializedPath == "/tmp/test-mixer");
    assert(device.initializedVolumes.size() == 1);
    assert(session.channels().size() == 1);
    assert(session.channels()[0].name == "line");
}

static void testRelativeChangeClampsStereoChannels() {
    NullLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("mic", 3, 250 + 256 * 4, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());

    assert(session.initialize() == 0);
    assert(session.changeChannelBy(0, 10) == 1);
    assert(device.setVolumeCalls == 1);
    assert(device.setVolumePath == "/dev/mixer-test");
    assert(device.setVolumeDeviceId == 3);
    assert(device.setVolumeEncodedVolume == 255 + 256 * 14);
    assert(session.channels()[0].encodedVolume == 255 + 256 * 14);

    assert(session.changeChannelBy(0, -30) == 1);
    assert(device.setVolumeEncodedVolume == 225);
    assert(session.channels()[0].encodedVolume == 225);
}

static void testAbsoluteValueUsesLegacyMonoShortcut() {
    NullLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("pcm", 4, 0, 0));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());

    assert(session.initialize() == 0);
    assert(session.setChannelValue(0, 17) == 1);
    assert(device.setVolumeEncodedVolume == 17 + 256 * 17);
    assert(session.channels()[0].encodedVolume == 17 + 256 * 17);

    assert(session.setChannelValue(0, 4000) == 1);
    assert(device.setVolumeEncodedVolume == 4000);
    assert(session.channels()[0].encodedVolume == 4000);
}

static void testSetFailureLeavesChannelStateStable() {
    NullLogSink log;
    FakeMixerDevice device;
    device.discoveredChannels.push_back(MixerChannel("line", 1, 20 + 256 * 30, 1));
    MixerSession session(device, log, "/dev/mixer-test",
        std::vector<MixerInitialVolume>());

    assert(session.initialize() == 0);
    device.setVolumeResult = 1;
    assert(session.changeChannelBy(0, 5) == 1);
    assert(device.setVolumeCalls == 1);
    assert(session.channels()[0].encodedVolume == 20 + 256 * 30);
    assert(session.channels()[0].active == 1);
}

int main() {
    testStartupConfigIsCopiedIntoSession();
    testRelativeChangeClampsStereoChannels();
    testAbsoluteValueUsesLegacyMonoShortcut();
    testSetFailureLeavesChannelStateStable();
    return 0;
}
