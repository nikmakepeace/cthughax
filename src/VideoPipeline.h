// Internal visual pipeline scaffold.

#ifndef __VIDEO_PIPELINE_H
#define __VIDEO_PIPELINE_H

#include "AudioFrame.h"
#include "AudioAnalyzer.h"

#include <vector>

class CthughaBuffer;
class FramePalette;

class VideoFrameContext {
public:
    const AudioFrame* audioFrame;
    const char2* rawAudioData;
    const char2* processedWaveData;
    const AudioMetrics* audioMetrics;
    const AcousticContext* acousticContext;
    double now;
    double deltaT;

    VideoFrameContext();
};

class VideoFrame {
    CthughaBuffer* bufferValue;
    const VideoFrameContext* contextValue;
    FramePalette* framePaletteValue;

public:
    VideoFrame(CthughaBuffer& buffer_, const VideoFrameContext& context_,
        FramePalette* framePalette_);

    CthughaBuffer& buffer();
    const VideoFrameContext& context() const;
    FramePalette* framePalette();
    const FramePalette* framePalette() const;
};

class VideoModule {
public:
    virtual ~VideoModule();

    virtual void refresh() { }
    virtual void execute(VideoFrame& frame) = 0;
};

enum VideoStageRunMode {
    VideoStageDisabled,
    VideoStageEnabled,
    // Executes on the next run, then the pipeline changes it to Disabled.
    VideoStageArmedOnce
};

class VideoPipeline {
    struct Entry {
        unsigned int stage;
        VideoModule* module;
        int owned;
        VideoStageRunMode mode;

        Entry(unsigned int stage_, VideoModule* module_, int owned_)
            : stage(stage_)
            , module(module_)
            , owned(owned_)
            , mode(VideoStageDisabled) { }
    };

    std::vector<Entry> modules;
    std::vector<unsigned int> sequence;
    FramePalette* framePaletteValue;

public:
    VideoPipeline();
    ~VideoPipeline();

    void clear();
    void add(unsigned int stage, VideoModule* module, int takeOwnership = 0);
    void setStageSequence(const std::vector<unsigned int>& stages);
    int moveStageBefore(unsigned int stage, unsigned int beforeStage);
    int moveStageAfter(unsigned int stage, unsigned int afterStage);
    int setStageMode(unsigned int stage, VideoStageRunMode mode);
    VideoStageRunMode stageMode(unsigned int stage) const;
    VideoModule* stageModule(unsigned int stage);
    void setFramePalette(FramePalette* framePalette);
    FramePalette* framePalette() const;
    void refresh();
    void run(CthughaBuffer& buffer, const VideoFrameContext& context);
    int size() const;
};

#endif
