// Frame Generator filterchain ownership.

#ifndef CTHUGHA_FRAME_FILTERCHAIN_PIPELINE_H
#define CTHUGHA_FRAME_FILTERCHAIN_PIPELINE_H

#include "VideoFilterchainFactory.h"

#include <memory>

class CthughaBuffer;
class FramePalette;
class IndexedFrame;
class VideoFrameContext;
class VideoFilterchain;
class VideoFilterchainSequence;

/**
 * Owns the concrete video filterchain used by FrameGeneratorRuntime.
 */
class FrameGeneratorPipeline {
    VideoFilterchainFactory factoryValue;
    std::unique_ptr<VideoFilterchain> filterchainValue;
    VideoFilterchainSequence sequenceValue;

public:
    /** Creates an empty pipeline. */
    FrameGeneratorPipeline();

    /** Destroys owned filters before returning. */
    ~FrameGeneratorPipeline();

    /**
     * Ensures the pipeline has been created from the provided sequence.
     *
     * @param sequence Stage sequence to install when the pipeline is empty.
     */
    void initialize(const VideoFilterchainSequence& sequence);

    /** Deletes the owned filterchain and published frame descriptor. */
    void reset();

    /** @return Owned filterchain. initialize() must have been called. */
    VideoFilterchain& filterchain();

    /** @return Palette owned by the pipeline's palette filter, or NULL. */
    FramePalette* framePalette() const;

    /**
     * Runs the owned filterchain against explicit storage and context.
     *
     * @param buffer Active/passive indexed storage to mutate.
     * @param context Borrowed inputs for this frame only.
     * @return Published frame descriptor owned by the filterchain.
     */
    const IndexedFrame& render(CthughaBuffer& buffer,
        const VideoFrameContext& context);
};

#endif
