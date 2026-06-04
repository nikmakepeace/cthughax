// Video filterchain policy.

#ifndef __VIDEO_DIRECTOR_H
#define __VIDEO_DIRECTOR_H

#include "Image.h"
#include "Option.h"
#include "Scene.h"
#include "SilenceMessage.h"
#include "VideoFilterchainSequence.h"

#include <string>

class CthughaBuffer;
struct SceneTransitionPolicy;
struct MessagesConfig;
class VideoFilterchain;

/**
 * Owns video filterchain policy for scene changes and visual cues.
 *
 * VideoDirector observes Scene changes, configures the concrete filters in a
 * VideoFilterchain, and arms one-shot image/text cue stages.
 */
class VideoDirector : public SceneObserver {
    ImageOption images;
    RandomLegalImagePlacementStrategy imagePlacementStrategy;
    SilenceMessage silenceMessage;
    Scene* scene;
    VideoFilterchain* filterchain;
    CthughaBuffer* buffer;
    unsigned int pendingSceneChanges;
    const IndexedImage* pendingImageCue;
    unsigned int pendingImageCueId;
    unsigned int appliedImageCueId;
    std::string pendingTextMessage;
    unsigned int pendingTextCueId;
    unsigned int appliedTextCueId;
    int pendingTextFrames;
    int pendingTextInkColor;

    /**
     * Applies pending scene settings to currently bound filters.
     *
     * @param changes Bitmask of SceneChange values describing what changed.
     */
    void applySceneToFilterchain(unsigned int changes);

    /** Arms a queued image cue when the filterchain and buffer are ready. */
    void applyPendingImageCue();

    /**
     * Applies the palette side effect chosen for an image cue.
     *
     * @param image Indexed image being injected; may carry its own source palette.
     */
    void applyImageCuePalette(const IndexedImage& image);

    /** Arms a queued text cue when the filterchain is ready. */
    void applyPendingTextCue();

public:
    VideoDirector();
    ~VideoDirector();

    void configureTransitions(const SceneTransitionPolicy& transitionPolicy);
    void configureQuietMessages(const MessagesConfig& messagesConfig);

    /**
     * Starts observing a scene for filterchain-affecting changes and cues.
     *
     * @param scene_ Scene to observe; not owned by the director.
     */
    void bindScene(Scene& scene_);

    /** Stops observing the current scene and clears pending cue/filterchain state. */
    void unbindScene();

    /** @return Image option collection used by scene commands and image loading. */
    ImageOption& imageOption();

    /** @return Quiet-message provider used for silence text cues. */
    SilenceMessage& silenceMessages();

    /**
     * Emits a quiet-message text cue after enough audio silence.
     *
     * @param quietLength Current quiet duration in milliseconds.
     * @return Nonzero when a text cue was emitted.
     */
    int observeQuiet(int quietLength);

    /**
     * Builds the default stage sequence for normal indexed rendering.
     *
     * @return Ordered image/border/flame/translate/wave/text/commit/palette/export stages.
     */
    VideoFilterchainSequence defaultFilterchainSequence() const;

    /**
     * Binds a filterchain to the current scene and returns the video buffer.
     *
     * Applies pending scene changes and any queued cues before returning.
     *
     * @param filterchain Filterchain to configure; not owned by the director.
     * @return Global CthughaBuffer used by the configured filterchain.
     */
    CthughaBuffer* configureFilterchain(VideoFilterchain& filterchain);

    /**
     * SceneObserver callback for settings changes.
     *
     * @param scene Scene that changed.
     * @param changes Bitmask of SceneChange values.
     */
    virtual void sceneChanged(Scene& scene, unsigned int changes);

    /**
     * SceneObserver callback for one-shot image/text cues.
     *
     * @param scene Scene that emitted the cue.
     * @param cue Cue payload. Text durations are measured in visual frames.
     */
    virtual void sceneCue(Scene& scene, const SceneCue& cue);
};

/** @return Process-wide video director singleton. */
VideoDirector& videoDirector();

/** Maximum quiet interval before a text cue, in milliseconds. */
extern OptionTime changeMsgTime;

#endif
