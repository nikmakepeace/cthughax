#ifndef __VIDEO_PIPELINE_FACTORY_H
#define __VIDEO_PIPELINE_FACTORY_H

#include "VideoPipelineSequence.h"

class VideoPipeline;

class VideoPipelineFactory {
public:
    VideoPipelineFactory();

    VideoPipeline* create(const VideoPipelineSequence& sequence) const;
    void refresh(VideoPipeline& pipeline, const VideoPipelineSequence& sequence) const;
};

#endif
