// Frame Generator module root.

#ifndef CTHUGHA_FRAME_GENERATOR_RUNTIME_H
#define CTHUGHA_FRAME_GENERATOR_RUNTIME_H

#include "FrameGeneratorPipeline.h"
#include "FrameGeneratorSceneBinding.h"
#include "FrameStore.h"

#include <string>
#include <vector>

class CountdownTimerFactory;
class FramePalette;
class IndexedFrame;
class LogSink;
class RandomSource;
class Scene;
class SceneGeometry;
class SilenceMessage;

/**
 * Startup settings consumed by FrameGeneratorRuntime.
 */
struct FrameGeneratorRuntimeConfig {
    PixelSize frameSize;
    double paletteSmoothingChance;
    int paletteSmoothSeconds;
    int quietMessageDurationMs;
    SilenceMessageConfig silenceMessages;

    /** Creates historical default generator settings. */
    FrameGeneratorRuntimeConfig();
};

/**
 * Application-owned root for indexed frame generation.
 *
 * FrameGeneratorRuntime owns frame geometry, source/destination storage,
 * filterchain state, scene binding, transition policy, and generator
 * diagnostics. All runtime collaborators are supplied through construction,
 * configuration, binding, or render context.
 */
class FrameGeneratorRuntime {
    RandomSource& randomSourceValue;
    LogSink& logValue;
    FrameGeometry geometryValue;
    FrameStore frameStoreValue;
    FrameTransitionController transitionControllerValue;
    FrameGeneratorSceneBinding sceneBindingValue;
    FrameGeneratorPipeline pipelineValue;
    FrameFilterchainSequence filterchainSequenceValue;
    std::vector<std::string> filterchainStageNamesValue;
    std::vector<int> filterchainStageEnabledValue;
    int filterchainStagePolicyActive;

    int filterchainStageEnabled(FrameFilterchainSequence::Stage stage) const;
    void applyFilterchainStageGates();

public:
    /**
     * Creates an unconfigured frame generator root.
     *
     * @param randomSource Random source owned by Application lifecycle.
     * @param timerFactory Timer factory used by quiet-message providers.
     * @param log Diagnostics sink owned by Application lifecycle.
     */
    FrameGeneratorRuntime(RandomSource& randomSource,
        CountdownTimerFactory& timerFactory, LogSink& log);

    /** Destroys filterchain and storage after dependents have been shut down. */
    ~FrameGeneratorRuntime();

    /**
     * Applies startup frame-generator settings.
     *
     * @param config Explicit generator settings translated by Application.
     */
    void configure(const FrameGeneratorRuntimeConfig& config);

    /** @return Current frame geometry. */
    const FrameGeometry& geometry() const;

    /** @return Scene-facing geometry port without pixel storage. */
    SceneGeometry& sceneGeometry();

    /** @return Owned frame store. */
    FrameStore& frameStore();

    /** @return Quiet-message provider used for silence text cues. */
    SilenceMessage& silenceMessages();

    /**
     * Updates the live probability that palette changes are smoothed.
     *
     * @param chance Probability clamped to 0..1.
     */
    void setPaletteSmoothingChance(double chance);

    /**
     * Updates the live filterchain stage sequence.
     *
     * @param stages Stable user-reorderable stage names. Structural tail stages
     *        are appended automatically.
     * @param enabled Per-stage enabled state; omitted entries are enabled.
     */
    void setFilterchainSequence(const std::vector<std::string>& stages,
        const std::vector<int>& enabled);

    /**
     * Updates live filterchain stage enable gates without changing sequence.
     *
     * @param stages Stable user-controlled stage names.
     * @param enabled Per-stage enabled state; omitted entries are enabled.
     */
    void setFilterchainStageEnabled(const std::vector<std::string>& stages,
        const std::vector<int>& enabled);

    /** @return Live palette-smoothing probability, 0..1. */
    double paletteSmoothingChance() const;

    /** @return Live palette-smoothing duration, in seconds. */
    int paletteSmoothSeconds() const;

    /**
     * Starts observing a Scene for generator-affecting changes.
     *
     * @param scene Scene to observe; not owned by the generator.
     */
    void bindScene(Scene& scene);

    /** Stops observing the bound Scene, if any. */
    void unbindScene();

    /**
     * Emits a quiet-message cue when policy says a quiet period is long enough.
     *
     * @param quietLength Current quiet duration in milliseconds.
     * @param quietMessageThresholdMs Explicit threshold from runtime option.
     * @param frameBudgetFramesPerSecond Explicit frame budget for cue duration.
     * @return Nonzero when a cue was emitted.
     */
    int observeQuiet(int quietLength, int quietMessageThresholdMs,
        int frameBudgetFramesPerSecond);

    /** Ensures the owned filterchain exists. */
    void initializePipeline();

    /** Deletes the owned filterchain. */
    void resetPipeline();

    /** @return Palette owned by the filterchain, or NULL before initialization. */
    FramePalette* framePalette();

    /**
     * Generates one indexed frame from explicit per-frame inputs.
     *
     * @param context Borrowed inputs and explicit transition frame budget.
     * @return Indexed frame descriptor valid until next render/resize/shutdown.
     */
    const IndexedFrame& render(const FrameGeneratorContext& context);
};

#endif
