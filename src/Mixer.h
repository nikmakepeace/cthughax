/** @file
 * Application-owned OSS mixer session and UI control adapters.
 */

#ifndef __MIXER_H
#define __MIXER_H

#include "Option.h"

#include <memory>
#include <stddef.h>
#include <string>
#include <vector>

class Interface;
class InterfaceElement;
class InterfaceElementOption;
class LogSink;
struct AudioConfig;

/** Startup mixer volume request copied from AudioConfig. */
struct MixerInitialVolume {
    std::string name;
    int encodedVolume;

    /** Creates an empty startup mixer volume request. */
    MixerInitialVolume();

    /**
     * Creates a startup mixer volume request.
     *
     * @param name_ Mixer channel name.
     * @param encodedVolume_ Encoded OSS left/right volume.
     */
    MixerInitialVolume(const std::string& name_, int encodedVolume_);
};

/** Runtime-visible OSS mixer channel state. */
struct MixerChannel {
    std::string name;
    int deviceId;
    int encodedVolume;
    int active;

    /** Creates an empty mixer channel. */
    MixerChannel();

    /**
     * Creates a mixer channel state snapshot.
     *
     * @param name_ Mixer channel name.
     * @param deviceId_ OSS mixer device id.
     * @param encodedVolume_ Encoded OSS left/right volume.
     * @param active_ Nonzero when selected as a recording source.
     */
    MixerChannel(const std::string& name_, int deviceId_, int encodedVolume_,
        int active_);
};

/** Hardware boundary for querying and mutating mixer state. */
class MixerDevice {
public:
    /** Destroys the mixer hardware boundary. */
    virtual ~MixerDevice();

    /**
     * Opens the mixer, applies startup volumes, and reports available channels.
     *
     * @param path OSS mixer device path.
     * @param initialVolumes Startup volume requests.
     * @param channels Receives discovered mixer channels on success.
     * @param log Diagnostic sink for hardware/open/ioctl messages.
     * @return Zero on success, nonzero when state should remain unchanged.
     */
    virtual int initialize(const std::string& path,
        const std::vector<MixerInitialVolume>& initialVolumes,
        std::vector<MixerChannel>& channels, LogSink& log) = 0;

    /**
     * Applies a channel volume and recording-source state.
     *
     * @param path OSS mixer device path.
     * @param channel Channel to mutate.
     * @param encodedVolume Encoded OSS left/right volume.
     * @param active Receives refreshed recording-source activity.
     * @param log Diagnostic sink for hardware/open/ioctl messages.
     * @return Zero on success, nonzero when session state should remain stable.
     */
    virtual int setVolume(const std::string& path, const MixerChannel& channel,
        int encodedVolume, int& active, LogSink& log) = 0;
};

/** Owned mixer state for the current application session. */
class MixerSession {
    MixerDevice& deviceValue;
    LogSink& logValue;
    std::string pathValue;
    std::vector<MixerInitialVolume> initialVolumesValue;
    std::vector<MixerChannel> channelsValue;

    int applyEncodedVolume(size_t index, int encodedVolume);

public:
    /**
     * Creates a mixer session from startup audio configuration.
     *
     * @param device Mixer hardware boundary to use.
     * @param log Diagnostic sink.
     * @param config Startup audio configuration.
     */
    MixerSession(MixerDevice& device, LogSink& log, const AudioConfig& config);

    /**
     * Creates a mixer session from explicit startup values.
     *
     * @param device Mixer hardware boundary to use.
     * @param log Diagnostic sink.
     * @param path OSS mixer device path.
     * @param initialVolumes Startup volume requests.
     */
    MixerSession(MixerDevice& device, LogSink& log, const std::string& path,
        const std::vector<MixerInitialVolume>& initialVolumes);

    /**
     * Initializes the session from the mixer device.
     *
     * @return Zero on success, nonzero when existing channel state is preserved.
     */
    int initialize();

    /** @return Startup mixer device path. */
    const std::string& path() const;

    /** @return Startup mixer volume requests. */
    const std::vector<MixerInitialVolume>& initialVolumes() const;

    /** @return Current mixer channels. */
    const std::vector<MixerChannel>& channels() const;

    /**
     * Changes a channel by a relative volume delta.
     *
     * @param index Channel index.
     * @param by Relative volume delta for both left and right channels.
     * @return Nonzero when the channel index belonged to this session.
     */
    int changeChannelBy(size_t index, int by);

    /**
     * Sets a channel from legacy option text semantics.
     *
     * @param index Channel index.
     * @param value Encoded value, or mono value when below 255.
     * @return Nonzero when the channel index belonged to this session.
     */
    int setChannelValue(size_t index, int value);
};

/** Option adapter collection for runtime mixer panel rows. */
class MixerControls {
    class MixerVolumeOption;

    MixerSession& sessionValue;
    LogSink& logValue;
    std::vector<std::unique_ptr<MixerVolumeOption> > optionsValue;
    std::vector<std::unique_ptr<char[]> > labelsValue;
    std::vector<std::unique_ptr<InterfaceElementOption> > elementsValue;
    std::vector<InterfaceElement*> elementPointersValue;

    int findOption(const Option& option) const;
    void rebuild();

public:
    /**
     * Creates mixer controls for a session.
     *
     * @param session Mixer session to mutate. The session must outlive controls.
     * @param log Diagnostic sink.
     */
    MixerControls(MixerSession& session, LogSink& log);

    /** Releases mixer option and interface row adapters. */
    ~MixerControls();

    /**
     * Installs mixer channel rows into the supplied interface.
     *
     * @param mixerInterface Interface panel to populate.
     */
    void installInto(Interface& mixerInterface);

    /**
     * Clears a previously installed mixer interface.
     *
     * @param mixerInterface Interface panel to clear.
     */
    void clearInterface(Interface& mixerInterface);

    /** @return Number of mixer option adapters. */
    size_t optionCount() const;

    /**
     * Returns a mixer option by index.
     *
     * @param index Mixer option index.
     * @return Option pointer, or NULL when out of range.
     */
    Option* optionAt(size_t index);

    /**
     * Attempts to change a mixer option by relative delta.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative volume delta.
     * @return Nonzero when the option belongs to this control set.
     */
    int changeOptionBy(Option& option, int by);

    /**
     * Attempts to change a mixer option from value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text.
     * @return Nonzero when the option belongs to this control set.
     */
    int changeOptionTo(Option& option, const char* to);
};

/**
 * Creates the platform mixer device boundary.
 *
 * @return Owned mixer device implementation for this build.
 */
MixerDevice* newMixerDevice();

#endif
