// Visual pipeline policy.

#ifndef __VISUAL_DIRECTOR_H
#define __VISUAL_DIRECTOR_H

#include "Image.h"
#include "VisualPipelineSequence.h"

class CthughaBuffer;
class VisualPipeline;

class VisualDirector {
    ImageOption images;
    RandomLegalImagePlacementStrategy imagePlacementStrategy;
    int imageLoadingEnabledValue;
    int lastImageSelection;

    int imageSelectionChanged() const;
    void markImageSelectionSeen();
    void syncCurrentBuffer();
    void updatePipelineStages(VisualPipeline& pipeline, CthughaBuffer& buffer);

public:
    VisualDirector();

    ImageOption& imageOption();
    int imageLoadingEnabled() const;
    void setImageLoadingEnabled(int enabled);
    int loadImages();

    VisualPipelineSequence defaultPipelineSequence() const;
    CthughaBuffer* configurePipeline(VisualPipeline& pipeline);
};

VisualDirector& visualDirector();

extern double paletteSmoothingChance;

#endif
