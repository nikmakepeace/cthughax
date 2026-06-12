#include "FrameFilterchainUserStages.h"

#include <assert.h>
#include <vector>

static void assertStageAt(const FrameFilterchainSequence& sequence,
    unsigned int index, FrameFilterchainSequence::Stage stage) {
    assert(sequence.sequence().size() > index);
    assert(sequence.sequence()[index] == unsigned(stage));
}

static void testArbitraryPixelOrderKeepsFixedTail() {
    std::vector<std::string> stages;
    stages.push_back("wave");
    stages.push_back("flame");
    stages.push_back("translate");
    stages.push_back("image");
    stages.push_back("border");
    stages.push_back("text");
    stages.push_back("palette");
    stages.push_back("flashlight");
    stages.push_back("indexedFrame");

    FrameFilterchainSequence sequence
        = frameFilterchainSequenceFromUserStageNames(stages);

    assert(sequence.sequence().size() == 10);
    assertStageAt(sequence, 0, FrameFilterchainSequence::WaveStage);
    assertStageAt(sequence, 1, FrameFilterchainSequence::FlameStage);
    assertStageAt(sequence, 2, FrameFilterchainSequence::TranslateStage);
    assertStageAt(sequence, 3, FrameFilterchainSequence::ImageStage);
    assertStageAt(sequence, 4, FrameFilterchainSequence::BorderStage);
    assertStageAt(sequence, 5, FrameFilterchainSequence::TextStage);
    assertStageAt(sequence, 6, FrameFilterchainSequence::FrameCommitStage);
    assertStageAt(sequence, 7, FrameFilterchainSequence::PaletteStage);
    assertStageAt(sequence, 8, FrameFilterchainSequence::FlashlightStage);
    assertStageAt(sequence, 9, FrameFilterchainSequence::IndexedFrameStage);
}

static void testDuplicatesAreStrippedAndMissingPixelStagesAppend() {
    std::vector<std::string> stages;
    stages.push_back("wave");
    stages.push_back("wave");
    stages.push_back("flame");

    FrameFilterchainSequence sequence
        = frameFilterchainSequenceFromUserStageNames(stages);

    assert(sequence.sequence().size() == 10);
    assertStageAt(sequence, 0, FrameFilterchainSequence::WaveStage);
    assertStageAt(sequence, 1, FrameFilterchainSequence::FlameStage);
    assertStageAt(sequence, 2, FrameFilterchainSequence::ImageStage);
    assertStageAt(sequence, 3, FrameFilterchainSequence::BorderStage);
    assertStageAt(sequence, 4, FrameFilterchainSequence::TranslateStage);
    assertStageAt(sequence, 5, FrameFilterchainSequence::TextStage);
    assertStageAt(sequence, 6, FrameFilterchainSequence::FrameCommitStage);
    assertStageAt(sequence, 7, FrameFilterchainSequence::PaletteStage);
    assertStageAt(sequence, 8, FrameFilterchainSequence::FlashlightStage);
    assertStageAt(sequence, 9, FrameFilterchainSequence::IndexedFrameStage);
}

static void testStructuralStagesAreNotUserStages() {
    FrameFilterchainSequence::Stage stage = FrameFilterchainSequence::ImageStage;
    assert(!frameFilterchainUserStageFromName("palette", &stage));
    assert(!frameFilterchainUserStageFromName("flashlight", &stage));
    assert(!frameFilterchainUserStageFromName("indexedFrame", &stage));
    assert(!frameFilterchainUserStageFromName("frameCommit", &stage));
}

int main() {
    testArbitraryPixelOrderKeepsFixedTail();
    testDuplicatesAreStrippedAndMissingPixelStagesAppend();
    testStructuralStagesAreNotUserStages();
    return 0;
}
