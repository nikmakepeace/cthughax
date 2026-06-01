// Visual pipeline policy.

#ifndef __VISUAL_DIRECTOR_H
#define __VISUAL_DIRECTOR_H

#include "Image.h"
#include "Scene.h"
#include "VisualPipelineSequence.h"

class CthughaBuffer;
class VisualPipeline;

class VisualDirector : public SceneObserver {
    ImageOption images;
    RandomLegalImagePlacementStrategy imagePlacementStrategy;
    Scene* scene;
    VisualPipeline* pipeline;
    CthughaBuffer* buffer;
    unsigned int pendingSceneChanges;
    const IndexedImage* pendingImageCue;
    unsigned int pendingImageCueId;
    unsigned int appliedImageCueId;
    int imageLoadingEnabledValue;

    void applySceneToPipeline(unsigned int changes);
    void applyPendingImageCue();

public:
    VisualDirector();
    ~VisualDirector();

    void bindScene(Scene& scene_);
    void unbindScene();

    ImageOption& imageOption();
    int imageLoadingEnabled() const;
    void setImageLoadingEnabled(int enabled);
    int loadImages();

    VisualPipelineSequence defaultPipelineSequence() const;
    CthughaBuffer* configurePipeline(VisualPipeline& pipeline);

    virtual void sceneChanged(Scene& scene, unsigned int changes);
    virtual void sceneCue(Scene& scene, const SceneCue& cue);
};

VisualDirector& visualDirector();

extern double paletteSmoothingChance;

#endif
