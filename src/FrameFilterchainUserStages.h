#ifndef CTHUGHA_FRAME_FILTERCHAIN_USER_STAGES_H
#define CTHUGHA_FRAME_FILTERCHAIN_USER_STAGES_H

#include "FrameFilterchainSequence.h"

#include <string>
#include <vector>

/**
 * Converts a control-panel/user stage name into a reorderable pixel stage.
 *
 * Structural tail stages are intentionally not accepted here; the framework
 * appends FrameCommit, Palette, Flashlight, and IndexedFrame itself.
 */
int frameFilterchainUserStageFromName(const std::string& name,
    FrameFilterchainSequence::Stage* stage);

/**
 * Builds a complete filterchain sequence from user-controlled pixel stages.
 *
 * User-visible pixel stages keep the supplied order. Missing pixel stages are
 * appended in default order, duplicates are stripped, and the structural tail is
 * always FrameCommit, Palette, Flashlight, IndexedFrame.
 */
FrameFilterchainSequence frameFilterchainSequenceFromUserStageNames(
    const std::vector<std::string>& stages);

#endif
