#include "FrameFilterchainSequence.h"

FrameFilterchainSequence::FrameFilterchainSequence() { }

void FrameFilterchainSequence::append(Stage stage) {
    sequenceValue.push_back(stage);
}

int FrameFilterchainSequence::includes(Stage stage) const {
    for (unsigned int i = 0; i < sequenceValue.size(); i++) {
        if (sequenceValue[i] == stage)
            return 1;
    }

    return 0;
}
