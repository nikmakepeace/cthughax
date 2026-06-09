// Frame Generator filterchain ownership.

#ifndef CTHUGHA_FRAME_GENERATOR_PIPELINE_H
#define CTHUGHA_FRAME_GENERATOR_PIPELINE_H

#include "FrameFilterchainFactory.h"

#include <memory>

class FrameRenderTarget;
class FramePalette;
class IndexedFrame;
class FrameGeneratorContext;
class FrameFilterchain;
class FrameFilterchainSequence;
class LogSink;

/**
 * Owns the concrete frame filterchain used by FrameGeneratorRuntime.
 */
class FrameGeneratorPipeline {
    FrameFilterchainFactory factoryValue;
    std::unique_ptr<FrameFilterchain> filterchainValue;
    FrameFilterchainSequence sequenceValue;

public:
    /**
     * Creates an empty pipeline with explicit diagnostics for owned filters.
     *
     * @param log Diagnostics sink supplied by FrameGeneratorRuntime.
     */
    explicit FrameGeneratorPipeline(LogSink& log);

    /** Destroys owned filters before returning. */
    ~FrameGeneratorPipeline();

    /**
     * Ensures the pipeline has been created from the provided sequence.
     *
     * @param sequence Stage sequence to install when the pipeline is empty.
     */
    void initialize(const FrameFilterchainSequence& sequence);

    /** Deletes the owned filterchain and published frame descriptor. */
    void reset();

    /** @return Owned filterchain. initialize() must have been called. */
    FrameFilterchain& filterchain();

    /** @return Palette owned by the pipeline's palette filter, or NULL. */
    FramePalette* framePalette() const;

    /**
     * Runs the owned filterchain against explicit storage and context.
     *
     * @param target Active/passive indexed render target to mutate.
     * @param context Borrowed inputs for this frame only.
     * @return Published frame descriptor owned by the filterchain.
     */
    const IndexedFrame& render(FrameRenderTarget& target,
        const FrameGeneratorContext& context);
};

#endif
