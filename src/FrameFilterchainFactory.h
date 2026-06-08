#ifndef CTHUGHA_FRAME_FILTERCHAIN_FACTORY_H
#define CTHUGHA_FRAME_FILTERCHAIN_FACTORY_H

#include "FrameFilterchainSequence.h"

class FrameFilterchain;

/**
 * Builds concrete frame filterchains from stage sequences.
 */
class FrameFilterchainFactory {
public:
    FrameFilterchainFactory();

    /**
     * Allocates and populates a filterchain for a sequence.
     *
     * @param sequence Stage order and set of filters to install.
     * @return Newly allocated filterchain owned by the caller.
     */
    FrameFilterchain* create(const FrameFilterchainSequence& sequence) const;

    /**
     * Refreshes filters after display or scene configuration changes.
     *
     * @param filterchain Existing filterchain to refresh.
     * @param sequence Stage sequence used for diagnostic context.
     */
    void refresh(FrameFilterchain& filterchain, const FrameFilterchainSequence& sequence) const;
};

#endif
