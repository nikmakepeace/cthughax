// Visual flashlight option.

#ifndef __FLASHLIGHT_H
#define __FLASHLIGHT_H

#include "CoreOption.h"

class CthughaBuffer;
class FramePalette;
class VideoFrameContext;

extern CoreOption flashlight;

void init_flashlight();
void apply_flashlight(FramePalette& framePalette, const VideoFrameContext& context);

#endif
