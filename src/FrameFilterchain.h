// Internal frame filterchain scaffold.

#ifndef CTHUGHA_FRAME_FILTERCHAIN_H
#define CTHUGHA_FRAME_FILTERCHAIN_H

#include "FrameGeneratorContext.h"
#include "IndexedFrame.h"

#include <vector>

class FrameRenderTarget;
class FramePalette;
class LogSink;

/**
 * Mutable frame object passed through one filterchain execution.
 *
 * Filters use this to access the active/passive indexed buffers, frame context,
 * optional frame palette, and final published IndexedFrame.
 */
class FrameFilterFrame {
    FrameRenderTarget* bufferValue;
    const FrameGeneratorContext* contextValue;
    FramePalette* framePaletteValue;
    IndexedFrame* indexedFrameValue;
    LogSink* logValue;

public:
    /**
     * Wraps the state for one frame filterchain run.
     *
     * @param buffer_ Active/passive indexed pixel buffer for this visual frame.
     * @param context_ Borrowed per-frame audio/time context.
     * @param framePalette_ Palette state to update or read, or NULL.
     * @param indexedFrame_ Destination for final display frame publication.
     * @param log_ Diagnostics sink for this filterchain run.
     */
    FrameFilterFrame(FrameRenderTarget& buffer_, const FrameGeneratorContext& context_,
        FramePalette* framePalette_, IndexedFrame* indexedFrame_, LogSink& log_);

    /** @return Active/passive indexed pixel buffer for this frame. */
    FrameRenderTarget& buffer();

    /** @return Borrowed audio/time context for this frame. */
    const FrameGeneratorContext& context() const;

    /** @return Mutable frame palette, or NULL when no palette stage is installed. */
    FramePalette* framePalette();

    /** @return Frame palette, or NULL when no palette stage is installed. */
    const FramePalette* framePalette() const;

    /**
     * Publishes the final indexed frame descriptor for display.
     *
     * @param indexedFrame Descriptor containing pixels, dimensions, pitch, and palette.
     */
    void publishIndexedFrame(const IndexedFrame& indexedFrame);

    /** @return Last IndexedFrame published during this filterchain run. */
    const IndexedFrame& indexedFrame() const;

    /** @return Diagnostics sink supplied by the owning FrameFilterchain. */
    LogSink& log() const;
};

/**
 * Base interface for one frame filter stage.
 */
class FrameFilter {
public:
    virtual ~FrameFilter();

    /** Rebuilds internal lookup/cache state after display or scene changes. */
    virtual void refresh() { }

    /**
     * Runs this filter for one visual frame.
     *
     * @param frame Mutable frame wrapper containing buffers, palette, and context.
     */
    virtual void execute(FrameFilterFrame& frame) = 0;
};

enum FrameFilterRunMode {
    /** Stage is skipped during filterchain runs. */
    FrameFilterDisabled,

    /** Stage executes on every filterchain run. */
    FrameFilterEnabled,

    /** Stage executes on the next run, then changes back to FrameFilterDisabled. */
    FrameFilterArmedOnce
};

/**
 * Ordered collection of frame filters keyed by stage id.
 *
 * Stage ids are normally FrameFilterchainSequence::Stage values. A stage can
 * contain more than one filter, and run modes are applied to every filter with
 * the matching stage id.
 */
class FrameFilterchain {
    struct Entry {
        unsigned int stage;
        FrameFilter* filter;
        int owned;
        FrameFilterRunMode mode;

        Entry(unsigned int stage_, FrameFilter* filter_, int owned_)
            : stage(stage_)
            , filter(filter_)
            , owned(owned_)
            , mode(FrameFilterDisabled) { }
    };

    std::vector<Entry> filters;
    std::vector<unsigned int> sequence;
    FramePalette* framePaletteValue;
    IndexedFrame indexedFrameValue;
    LogSink* logValue;

public:
    /**
     * Creates an empty filterchain with an explicit diagnostics sink.
     *
     * @param log Diagnostics sink used by filterchain bookkeeping and filters.
     */
    explicit FrameFilterchain(LogSink& log);
    ~FrameFilterchain();

    /** Deletes owned filters and clears stage order, palette, and published frame. */
    void clear();

    /**
     * Adds a filter to a stage.
     *
     * @param stage Stage id, normally a FrameFilterchainSequence::Stage value.
     * @param filter Filter instance to add; ignored when NULL.
     * @param takeOwnership Nonzero to delete filter in clear()/destructor.
     */
    void add(unsigned int stage, FrameFilter* filter, int takeOwnership = 0);

    /**
     * Replaces the stage execution order.
     *
     * @param stages Ordered stage ids to visit each run.
     */
    void setStageSequence(const std::vector<unsigned int>& stages);

    /**
     * Moves one stage before another in the current sequence.
     *
     * @param stage Stage id to move.
     * @param beforeStage Existing stage id to move before.
     * @return Nonzero on success, zero if either stage is absent.
     */
    int moveStageBefore(unsigned int stage, unsigned int beforeStage);

    /**
     * Moves one stage after another in the current sequence.
     *
     * @param stage Stage id to move.
     * @param afterStage Existing stage id to move after.
     * @return Nonzero on success, zero if either stage is absent.
     */
    int moveStageAfter(unsigned int stage, unsigned int afterStage);

    /**
     * Sets the run mode for every filter registered under a stage.
     *
     * @param stage Stage id to update.
     * @param mode Disabled, enabled, or armed-once execution mode.
     * @return Number of filters matched by the stage id.
     */
    int setStageMode(unsigned int stage, FrameFilterRunMode mode);

    /**
     * @param stage Stage id to query.
     * @return First matching filter's run mode, or FrameFilterDisabled.
     */
    FrameFilterRunMode stageMode(unsigned int stage) const;

    /**
     * @param stage Stage id to query.
     * @return First matching filter for the stage, or NULL.
     */
    FrameFilter* stageFilter(unsigned int stage);

    /**
     * Sets the shared frame palette pointer supplied to FrameFilterFrame.
     *
     * @param framePalette Palette owned by a filter or caller; not owned here.
     */
    void setFramePalette(FramePalette* framePalette);

    /** @return Shared frame palette pointer, or NULL. */
    FramePalette* framePalette() const;

    /** @return Most recent final indexed frame descriptor published by filters. */
    const IndexedFrame& indexedFrame() const;

    /** Calls refresh() on every registered filter. */
    void refresh();

    /**
     * Executes enabled filters in configured stage order for one visual frame.
     *
     * @param buffer Active/passive indexed pixel buffer to mutate.
     * @param context Per-frame audio/time context; borrowed during the call.
     */
    void run(FrameRenderTarget& buffer, const FrameGeneratorContext& context);

    /** @return Number of registered filter entries, not number of stages. */
    int size() const;
};

#endif
