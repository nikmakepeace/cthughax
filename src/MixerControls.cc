/** @file
 * Runtime Option adapters for the OSS mixer panel.
 */

#include "Mixer.h"
#include "Interface.h"
#include "ProcessServices.h"

#include <stdio.h>
#include <stdlib.h>

class MixerControls::MixerVolumeOption : public Option {
    MixerSession& sessionValue;
    LogSink& logValue;
    size_t channelIndex;
    mutable char textValue[128];

    void syncValue() {
        const std::vector<MixerChannel>& channels = sessionValue.channels();
        value = (channelIndex < channels.size())
            ? channels[channelIndex].encodedVolume
            : 0;
    }

public:
    MixerVolumeOption(MixerSession& session, LogSink& log, size_t channelIndex_)
        : Option(NULL)
        , sessionValue(session)
        , logValue(log)
        , channelIndex(channelIndex_)
        , textValue() {
        syncValue();
    }

    virtual void setValue(int value_) {
        sessionValue.setChannelValue(channelIndex, value_);
        syncValue();
    }

    virtual void change(int by) {
        sessionValue.changeChannelBy(channelIndex, by);
        syncValue();
    }

    virtual void change(const char* to) {
        char* end = NULL;
        long parsed = strtol(to, &end, 0);
        if ((to == end) || ((end != NULL) && (*end != '\0'))) {
            logValue.error("Not a mixer volume value `%s'.\n", to);
            return;
        }
        sessionValue.setChannelValue(channelIndex, int(parsed));
        syncValue();
    }

    virtual const char* text() const {
        const std::vector<MixerChannel>& channels = sessionValue.channels();
        if (channelIndex >= channels.size())
            return "";

        const MixerChannel& channel = channels[channelIndex];
        snprintf(textValue, sizeof(textValue), "%3d:%-3d%c",
            channel.encodedVolume % 256, channel.encodedVolume / 256,
            channel.active ? '*' : ' ');
        return textValue;
    }
};

MixerControls::MixerControls(MixerSession& session, LogSink& log)
    : sessionValue(session)
    , logValue(log)
    , optionsValue()
    , labelsValue()
    , elementsValue()
    , elementPointersValue() {
    rebuild();
}

MixerControls::~MixerControls() { }

void MixerControls::rebuild() {
    optionsValue.clear();
    labelsValue.clear();
    elementsValue.clear();
    elementPointersValue.clear();

    const std::vector<MixerChannel>& channels = sessionValue.channels();
    optionsValue.reserve(channels.size());

    for (size_t i = 0; i < channels.size(); i++) {
        optionsValue.push_back(
            std::unique_ptr<MixerVolumeOption>(
                new MixerVolumeOption(sessionValue, logValue, i)));
    }
}

size_t MixerControls::optionCount() const {
    return optionsValue.size();
}

Option* MixerControls::optionAt(size_t index) {
    if (index >= optionsValue.size())
        return NULL;
    return optionsValue[index].get();
}

int MixerControls::findOption(const Option& option) const {
    for (size_t i = 0; i < optionsValue.size(); i++) {
        if (optionsValue[i].get() == &option)
            return int(i);
    }
    return -1;
}

int MixerControls::changeOptionBy(Option& option, int by) {
    int index = findOption(option);
    if (index < 0)
        return 0;

    optionsValue[size_t(index)]->change(by);
    return 1;
}

int MixerControls::changeOptionTo(Option& option, const char* to) {
    int index = findOption(option);
    if (index < 0)
        return 0;

    optionsValue[size_t(index)]->change(to);
    return 1;
}
