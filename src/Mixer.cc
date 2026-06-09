/** @file
 * Application-owned OSS mixer session and UI control adapters.
 */
#include "config.h"
#include "Mixer.h"
#include "Configuration.h"
#include "ProcessServices.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if WITH_MIXER == 1

// include the right soundcard.h
#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

static int clampMixerChannelVolume(int value) {
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return value;
}

MixerInitialVolume::MixerInitialVolume()
    : name()
    , encodedVolume(0) { }

MixerInitialVolume::MixerInitialVolume(const std::string& name_,
    int encodedVolume_)
    : name(name_)
    , encodedVolume(encodedVolume_) { }

MixerChannel::MixerChannel()
    : name()
    , deviceId(0)
    , encodedVolume(0)
    , active(0) { }

MixerChannel::MixerChannel(const std::string& name_, int deviceId_,
    int encodedVolume_, int active_)
    : name(name_)
    , deviceId(deviceId_)
    , encodedVolume(encodedVolume_)
    , active(active_ ? 1 : 0) { }

MixerDevice::~MixerDevice() { }

MixerSession::MixerSession(MixerDevice& device, LogSink& log,
    const AudioConfig& config)
    : deviceValue(device)
    , logValue(log)
    , pathValue(config.mixerDevicePath)
    , initialVolumesValue()
    , channelsValue() {
    for (std::vector<AudioMixerInitialVolumeConfig>::const_iterator it
         = config.mixerInitialVolumes.begin();
         it != config.mixerInitialVolumes.end(); ++it) {
        initialVolumesValue.push_back(
            MixerInitialVolume(it->name, it->volume));
    }
}

MixerSession::MixerSession(MixerDevice& device, LogSink& log,
    const std::string& path,
    const std::vector<MixerInitialVolume>& initialVolumes)
    : deviceValue(device)
    , logValue(log)
    , pathValue(path)
    , initialVolumesValue(initialVolumes)
    , channelsValue() { }

int MixerSession::initialize() {
    std::vector<MixerChannel> channels;
    int result = deviceValue.initialize(pathValue, initialVolumesValue, channels,
        logValue);
    if (result == 0)
        channelsValue = channels;
    return result;
}

const std::string& MixerSession::path() const {
    return pathValue;
}

const std::vector<MixerInitialVolume>& MixerSession::initialVolumes() const {
    return initialVolumesValue;
}

const std::vector<MixerChannel>& MixerSession::channels() const {
    return channelsValue;
}

int MixerSession::applyEncodedVolume(size_t index, int encodedVolume) {
    if (index >= channelsValue.size())
        return 0;

    if (encodedVolume < 0)
        encodedVolume = 0;

    int active = encodedVolume > 0;
    if (deviceValue.setVolume(pathValue, channelsValue[index], encodedVolume,
            active, logValue) != 0)
        return 1;

    channelsValue[index].encodedVolume = encodedVolume;
    channelsValue[index].active = active ? 1 : 0;
    return 1;
}

int MixerSession::changeChannelBy(size_t index, int by) {
    if (index >= channelsValue.size())
        return 0;

    int encodedVolume = channelsValue[index].encodedVolume;
    int left = encodedVolume % 256;
    int right = encodedVolume / 256;

    left = clampMixerChannelVolume(left + by);
    right = clampMixerChannelVolume(right + by);

    return applyEncodedVolume(index, left + 256 * right);
}

int MixerSession::setChannelValue(size_t index, int value) {
    if (index >= channelsValue.size())
        return 0;

    if (value < 0)
        value = 0;

    int encodedVolume = (value < 255) ? value * 256 + value : value;
    return applyEncodedVolume(index, encodedVolume);
}

class NullMixerDevice : public MixerDevice {
public:
    virtual int initialize(const std::string& path,
        const std::vector<MixerInitialVolume>& initialVolumes,
        std::vector<MixerChannel>& channels, LogSink& log) {
        (void)path;
        channels.clear();
        if (!initialVolumes.empty())
            log.warn("mixer was disabled at compile time.\n");
        return 0;
    }

    virtual int setVolume(const std::string& path, const MixerChannel& channel,
        int encodedVolume, int& active, LogSink& log) {
        (void)path;
        (void)channel;
        (void)log;
        active = encodedVolume > 0;
        return 0;
    }
};

#if WITH_MIXER == 1

class OssMixerDevice : public MixerDevice {
    static const char* mixerName(int index) {
        static const char* mixerNames[] = SOUND_DEVICE_NAMES;
        return mixerNames[index];
    }

    static int initialVolumeForDevice(
        const std::vector<MixerInitialVolume>& initialVolumes, int deviceId,
        int& specified) {
        for (std::vector<MixerInitialVolume>::const_iterator it
             = initialVolumes.begin();
             it != initialVolumes.end(); ++it) {
            if (strcasecmp(mixerName(deviceId), it->name.c_str()) == 0) {
                specified = 1;
                return it->encodedVolume;
            }
        }

        specified = 0;
        return -1;
    }

    static void reportUnknownInitialVolumes(
        const std::vector<MixerInitialVolume>& initialVolumes, LogSink& log) {
        for (std::vector<MixerInitialVolume>::const_iterator it
             = initialVolumes.begin();
             it != initialVolumes.end(); ++it) {
            int found = 0;
            for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
                if (strcasecmp(mixerName(i), it->name.c_str()) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found)
                log.error("unknown mixer device `%s'.\n", it->name.c_str());
        }
    }

public:
    virtual int initialize(const std::string& path,
        const std::vector<MixerInitialVolume>& initialVolumes,
        std::vector<MixerChannel>& channels, LogSink& log) {
        channels.clear();
        reportUnknownInitialVolumes(initialVolumes, log);

        if (initialVolumes.empty())
            log.debug("  no initial volumes specified.\n");

        int mixerDescriptor = open(path.c_str(), O_RDONLY);
        if (mixerDescriptor < 0) {
            if ((errno == ENOENT) || (errno == ENODEV)) {
                if (!initialVolumes.empty()) {
                    log.warn("  OSS mixer `%s' is unavailable; mixer options will be ignored.\n",
                        path.c_str());
                } else {
                    log.debug("  OSS mixer `%s' is unavailable; skipping mixer initialization.\n",
                        path.c_str());
                }
                return 0;
            }
            log.errorErrno(errno, "Can not open `%s'.", path.c_str());
            return 0;
        }

        int devMask = 0;
        if (ioctl(mixerDescriptor, SOUND_MIXER_READ_DEVMASK, &devMask) < 0)
            log.errorErrno(errno, "Can not get mixer device mask.");

        int mixerMask = 0;
        if (ioctl(mixerDescriptor, MIXER_READ(SOUND_MIXER_RECSRC),
                &mixerMask)
            < 0)
            log.errorErrno(errno, "Can not get recording source mask.");

        for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
            int specified = 0;
            int encodedVolume = initialVolumeForDevice(initialVolumes, i,
                specified);

            if ((devMask & (1 << i)) == 0) {
                if (specified)
                    log.warn("  unavailable mixer device `%s'.\n",
                        mixerName(i));
                continue;
            }

            if (!specified) {
                if (ioctl(mixerDescriptor, MIXER_READ(i), &encodedVolume) < 0) {
                    log.errorErrno(errno, "Can not get mixer value for `%s'.",
                        mixerName(i));
                    encodedVolume = 0;
                }
            } else {
                if (encodedVolume == 0) {
                    mixerMask &= ~(1 << i);
                    log.debug("    disabling `%s' for input.\n",
                        mixerName(i));
                } else {
                    log.debug("    setting `%s' to volume %d.\n",
                        mixerName(i), encodedVolume & 255);
                    mixerMask |= 1 << i;
                }

                if (ioctl(mixerDescriptor, MIXER_WRITE(i), &encodedVolume) < 0)
                    log.errorErrno(errno, "Can not set mixer value for `%s'.",
                        mixerName(i));
            }

            channels.push_back(MixerChannel(mixerName(i), i, encodedVolume,
                (mixerMask & (1 << i)) != 0));
        }

        if (ioctl(mixerDescriptor, MIXER_WRITE(SOUND_MIXER_RECSRC),
                &mixerMask)
            < 0)
            log.errorErrno(errno, "Can not set recording source");

        close(mixerDescriptor);

        log.debug("available mixer devices: %d.\n", int(channels.size()));
        return 0;
    }

    virtual int setVolume(const std::string& path, const MixerChannel& channel,
        int encodedVolume, int& active, LogSink& log) {
        int mixerDescriptor = open(path.c_str(), O_RDWR);
        if (mixerDescriptor < 0) {
            log.errorErrno(errno, "Can not open `%s'.", path.c_str());
            return 1;
        }

        int mixerMask = 0;
        if (ioctl(mixerDescriptor, MIXER_READ(SOUND_MIXER_RECSRC),
                &mixerMask)
            < 0) {
            log.errorErrno(errno, "Can not get recording source mask.");
            close(mixerDescriptor);
            return 1;
        }

        if (encodedVolume <= 0) {
            encodedVolume = 0;
            mixerMask &= ~(1 << channel.deviceId);
        } else {
            mixerMask |= (1 << channel.deviceId);
        }

        if (ioctl(mixerDescriptor, MIXER_WRITE(SOUND_MIXER_RECSRC),
                &mixerMask)
            < 0) {
            log.errorErrno(errno, "Can not set recording source mask");
            close(mixerDescriptor);
            return 1;
        }

        if (ioctl(mixerDescriptor, MIXER_READ(SOUND_MIXER_RECSRC),
                &mixerMask)
            < 0) {
            log.errorErrno(errno, "Can not get recording source mask.");
            close(mixerDescriptor);
            return 1;
        }

        active = (mixerMask & (1 << channel.deviceId)) != 0;

        int value = encodedVolume;
        if (ioctl(mixerDescriptor, MIXER_WRITE(channel.deviceId), &value) < 0) {
            log.errorErrno(errno, "Can not set mixer value for `%s'.",
                channel.name.c_str());
            close(mixerDescriptor);
            return 1;
        }

        close(mixerDescriptor);
        return 0;
    }
};

#endif

MixerDevice* newMixerDevice() {
#if WITH_MIXER == 1
    return new OssMixerDevice();
#else
    return new NullMixerDevice();
#endif
}
