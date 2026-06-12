/** @file
 * Runtime frame-generator control adapter.
 */

#include "RuntimeFrameGeneratorControls.h"

#include "FrameGeneratorRuntime.h"

DefaultRuntimeFrameGeneratorControls::DefaultRuntimeFrameGeneratorControls(
    FrameGeneratorRuntime& frameGenerator_)
    : frameGenerator(frameGenerator_) { }

void DefaultRuntimeFrameGeneratorControls::changePaletteSmoothingChanceTo(
    double chance) {
    frameGenerator.setPaletteSmoothingChance(chance);
}

void DefaultRuntimeFrameGeneratorControls::changeFilterchainSequenceTo(
    const std::vector<std::string>& stages,
    const std::vector<int>& enabled) {
    frameGenerator.setFilterchainSequence(stages, enabled);
}

void DefaultRuntimeFrameGeneratorControls::changeFilterchainEnabledTo(
    const std::vector<std::string>& stages,
    const std::vector<int>& enabled) {
    frameGenerator.setFilterchainStageEnabled(stages, enabled);
}
