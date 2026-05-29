// Visual pipeline selection scaffold.

#ifndef __VISUAL_DIRECTOR_H
#define __VISUAL_DIRECTOR_H

#include "VisualPipeline.h"

#include <vector>

class Environment;
class Settings;

class VisualPlan {
    std::vector<unsigned int> sequenceValue;

public:
    enum Stage {
        ImageStage,
        FlashlightStage,
        BorderStage,
        FlameStage,
        TranslateStage,
        WaveStage,
        FrameCommitStage,
        PaletteStage
    };

    VisualPlan();

    void append(Stage stage);
    int includes(Stage stage) const;
    const std::vector<unsigned int>& sequence() const { return sequenceValue; }
};

class VisualDirector {
    std::vector<int> pcxSelectionByBuffer;

    int pcxSelectionChanged();
    void syncSelectedBuffer();
    void bindPipelineStages(VisualPipeline& pipeline);

public:
    VisualPlan planDefaultPipeline() const;
    void configurePipeline(VisualPipeline& pipeline);
};

extern double paletteSmoothingChance;

class VisualPipelineFactory {
public:
    VisualPipelineFactory();

    VisualPipeline* create(const VisualPlan& plan) const;
    void refresh(VisualPipeline& pipeline, const VisualPlan& plan) const;
};

#endif
