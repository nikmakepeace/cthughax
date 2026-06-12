#ifndef CTHUGHA_FRAME_FILTERCHAIN_SEQUENCE_H
#define CTHUGHA_FRAME_FILTERCHAIN_SEQUENCE_H

#include <vector>

/**
 * Ordered list of frame filter stages used when constructing a filterchain.
 */
class FrameFilterchainSequence {
    std::vector<unsigned int> sequenceValue;

public:
    /** Named stage ids understood by FrameFilterchainFactory and Frame Generator. */
    enum Stage {
        /** One-shot indexed image injection into source/destination buffers. */
        ImageStage,

        /** Fixed-tail palette flashlight adjustment driven by acoustic context. */
        FlashlightStage,

        /** Hidden border rows used by flame feedback. */
        BorderStage,

        /** Flame feedback stage. */
        FlameStage,

        /** Coordinate remap/translation stage. */
        TranslateStage,

        /** Sound-reactive wave drawing stage. */
        WaveStage,

        /** Text cue injection stage. */
        TextStage,

        /** Framework commit boundary and frame diagnostics. */
        FrameCommitStage,

        /** Fixed-tail palette transition stage. */
        PaletteStage,

        /** Fixed-tail IndexedFrame publication stage. */
        IndexedFrameStage
    };

    FrameFilterchainSequence();

    /**
     * Appends a stage to the execution order.
     *
     * @param stage Stage id to append.
     */
    void append(Stage stage);

    /**
     * @param stage Stage id to search for.
     * @return Nonzero when the stage appears in the sequence.
     */
    int includes(Stage stage) const;

    /** @return Ordered stage ids as unsigned ints for FrameFilterchain. */
    const std::vector<unsigned int>& sequence() const { return sequenceValue; }
};

#endif
