#include "FrameFilterchainUserStages.h"

int frameFilterchainUserStageFromName(const std::string& name,
    FrameFilterchainSequence::Stage* stage) {
    if (name == "image") {
        *stage = FrameFilterchainSequence::ImageStage;
        return 1;
    }
    if (name == "border") {
        *stage = FrameFilterchainSequence::BorderStage;
        return 1;
    }
    if (name == "flame") {
        *stage = FrameFilterchainSequence::FlameStage;
        return 1;
    }
    if (name == "translate") {
        *stage = FrameFilterchainSequence::TranslateStage;
        return 1;
    }
    if (name == "wave") {
        *stage = FrameFilterchainSequence::WaveStage;
        return 1;
    }
    if (name == "text") {
        *stage = FrameFilterchainSequence::TextStage;
        return 1;
    }
    return 0;
}

static void appendUniqueStage(FrameFilterchainSequence& sequence,
    FrameFilterchainSequence::Stage stage) {
    if (!sequence.includes(stage))
        sequence.append(stage);
}

static void appendDefaultUserStages(FrameFilterchainSequence& sequence) {
    appendUniqueStage(sequence, FrameFilterchainSequence::ImageStage);
    appendUniqueStage(sequence, FrameFilterchainSequence::BorderStage);
    appendUniqueStage(sequence, FrameFilterchainSequence::FlameStage);
    appendUniqueStage(sequence, FrameFilterchainSequence::TranslateStage);
    appendUniqueStage(sequence, FrameFilterchainSequence::WaveStage);
    appendUniqueStage(sequence, FrameFilterchainSequence::TextStage);
}

FrameFilterchainSequence frameFilterchainSequenceFromUserStageNames(
    const std::vector<std::string>& stages) {
    FrameFilterchainSequence sequence;
    for (std::vector<std::string>::const_iterator it = stages.begin();
         it != stages.end(); ++it) {
        FrameFilterchainSequence::Stage stage
            = FrameFilterchainSequence::ImageStage;
        if (frameFilterchainUserStageFromName(*it, &stage))
            appendUniqueStage(sequence, stage);
    }

    appendDefaultUserStages(sequence);
    sequence.append(FrameFilterchainSequence::FrameCommitStage);
    sequence.append(FrameFilterchainSequence::PaletteStage);
    sequence.append(FrameFilterchainSequence::FlashlightStage);
    sequence.append(FrameFilterchainSequence::IndexedFrameStage);
    return sequence;
}
