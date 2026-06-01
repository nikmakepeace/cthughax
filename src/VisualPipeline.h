// Internal visual pipeline scaffold.

#ifndef __VISUAL_PIPELINE_H
#define __VISUAL_PIPELINE_H

#include "AudioFrame.h"
#include "AudioAnalyzer.h"

#include <vector>

class CthughaBuffer;

class VisualFrameContext {
public:
    const AudioFrame* audioFrame;
    const char2* rawAudioData;
    const char2* processedWaveData;
    const AudioMetrics* audioMetrics;
    const AcousticContext* acousticContext;
    double now;
    double deltaT;

    VisualFrameContext();
};

class VisualModule {
public:
    virtual ~VisualModule();

    virtual void refresh() { }
    virtual void execute(CthughaBuffer& buffer, const VisualFrameContext& context) = 0;
};

enum VisualStageRunMode {
    VisualStageDisabled,
    VisualStageEnabled,
    // Executes on the next run, then the pipeline changes it to Disabled.
    VisualStageArmedOnce
};

class VisualPipeline {
    struct Entry {
        unsigned int stage;
        VisualModule* module;
        int owned;
        VisualStageRunMode mode;

        Entry(unsigned int stage_, VisualModule* module_, int owned_)
            : stage(stage_)
            , module(module_)
            , owned(owned_)
            , mode(VisualStageDisabled) { }
    };

    std::vector<Entry> modules;
    std::vector<unsigned int> sequence;

public:
    VisualPipeline();
    ~VisualPipeline();

    void clear();
    void add(unsigned int stage, VisualModule* module, int takeOwnership = 0);
    void setStageSequence(const std::vector<unsigned int>& stages);
    int moveStageBefore(unsigned int stage, unsigned int beforeStage);
    int moveStageAfter(unsigned int stage, unsigned int afterStage);
    int setStageMode(unsigned int stage, VisualStageRunMode mode);
    VisualStageRunMode stageMode(unsigned int stage) const;
    VisualModule* stageModule(unsigned int stage);
    void refresh();
    void run(CthughaBuffer& buffer, const VisualFrameContext& context);
    int size() const;
};

#endif
