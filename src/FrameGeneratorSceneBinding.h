// Scene binding for the Frame Generator module.

#ifndef CTHUGHA_FRAME_GENERATOR_SCENE_BINDING_H
#define CTHUGHA_FRAME_GENERATOR_SCENE_BINDING_H

#include "FrameGeneratorContext.h"
#include "FrameGeometry.h"
#include "FrameTransitionController.h"
#include "ImagePlacement.h"
#include "Scene.h"
#include "SilenceMessage.h"
#include "FrameFilterchainSequence.h"

#include <string>

class CountdownTimerFactory;
class RandomSource;
class FrameFilterchain;
class LogSink;

/**
 * Observes Scene changes and applies queued generator work to a filterchain.
 *
 * This binding owns image/text cue queues and filter configuration decisions,
 * but it does not own scene selection policy or pixel storage.
 */
class FrameGeneratorSceneBinding : public SceneObserver {
    const FrameGeometry& geometryValue;
    FrameTransitionController& transitionController;
    RandomLegalImagePlacementStrategy imagePlacementStrategy;
    RandomSource& randomSourceValue;
    LogSink& logValue;
    SilenceMessage silenceMessage;
    Scene* scene;
    FrameFilterchain* filterchain;
    unsigned int pendingSceneChanges;
    const IndexedImage* pendingImageCue;
    unsigned int pendingImageCueId;
    unsigned int appliedImageCueId;
    std::string pendingTextMessage;
    unsigned int pendingTextCueId;
    unsigned int appliedTextCueId;
    int pendingTextFrames;
    int pendingTextInkColor;

    void applySceneToFilterchain(unsigned int changes,
        int frameBudgetFramesPerSecond);
    void applyPendingImageCue(int frameBudgetFramesPerSecond);
    void applyImageCuePalette(const IndexedImage& image,
        int frameBudgetFramesPerSecond);
    void applyPendingTextCue();

public:
    /**
     * Creates a scene binding with explicit runtime collaborators.
     *
     * @param geometry Frame dimensions used for image placement.
     * @param transitionController Runtime transition policy.
     * @param randomSource Random source used for cue placement/palette policy.
     * @param timerFactory Timer factory used by quiet-message providers.
     * @param log Diagnostics sink owned by the application lifecycle.
     */
    FrameGeneratorSceneBinding(const FrameGeometry& geometry,
        FrameTransitionController& transitionController,
        RandomSource& randomSource, CountdownTimerFactory& timerFactory,
        LogSink& log);

    /** Stops observing any bound scene. */
    ~FrameGeneratorSceneBinding();

    /** @return Quiet-message provider used for silence text cues. */
    SilenceMessage& silenceMessages();

    /**
     * Starts observing a scene for filterchain-affecting changes and cues.
     *
     * @param scene_ Scene to observe; not owned by the binding.
     */
    void bindScene(Scene& scene_);

    /** Stops observing the current scene and clears pending cue/filterchain state. */
    void unbindScene();

    /**
     * Emits a quiet-message text cue after enough audio silence.
     *
     * @param quietLength Current quiet duration in milliseconds.
     * @param quietMessageThresholdMs Runtime threshold for quiet messages.
     * @param frameBudgetFramesPerSecond Explicit frame budget for cue duration.
     * @return Nonzero when a text cue was emitted.
     */
    int observeQuiet(int quietLength, int quietMessageThresholdMs,
        int frameBudgetFramesPerSecond);

    /**
     * Builds the default stage sequence for normal indexed rendering.
     *
     * @return Ordered image/border/flame/translate/wave/text/commit/palette/export stages.
     */
    FrameFilterchainSequence defaultFilterchainSequence() const;

    /**
     * Applies pending scene changes and cues to a filterchain.
     *
     * @param filterchain_ Filterchain owned by FrameGeneratorPipeline.
     * @param context Explicit per-frame generator context.
     */
    void configureFilterchain(FrameFilterchain& filterchain_,
        const FrameGeneratorContext& context);

    /** SceneObserver callback for settings changes. */
    virtual void sceneChanged(Scene& scene, unsigned int changes);

    /** SceneObserver callback for one-shot image/text cues. */
    virtual void sceneCue(Scene& scene, const SceneCue& cue);
};

#endif
