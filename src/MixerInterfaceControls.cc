/** @file
 * Legacy Interface panel installation for owned mixer controls.
 */

#include "Mixer.h"
#include "Interface.h"

#include <stdio.h>
#include <utility>

void MixerControls::installInto(Interface& mixerInterface) {
    rebuild();
    labelsValue.reserve(optionsValue.size());
    elementsValue.reserve(optionsValue.size());
    elementPointersValue.reserve(optionsValue.size());

    const std::vector<MixerChannel>& channels = sessionValue.channels();
    for (size_t i = 0; i < optionsValue.size(); i++) {
        std::unique_ptr<char[]> label(new char[128]);
        snprintf(label.get(), 128, "%10s       : %%s",
            channels[i].name.c_str());
        labelsValue.push_back(std::move(label));

        elementsValue.push_back(
            std::unique_ptr<InterfaceElementOption>(
                new InterfaceElementOption(labelsValue.back().get(),
                    optionAt(i), 1, 10, 255)));
        elementPointersValue.push_back(elementsValue.back().get());
    }

    mixerInterface.setElements(elementPointersValue.empty()
            ? NULL
            : &elementPointersValue[0],
        int(elementPointersValue.size()));
}

void MixerControls::clearInterface(Interface& mixerInterface) {
    mixerInterface.setElements(NULL, 0);
}
