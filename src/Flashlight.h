// Visual flashlight option.

#ifndef __FLASHLIGHT_H
#define __FLASHLIGHT_H

#include "CoreOption.h"

class CthughaBuffer;
class VisualFrameContext;

extern CoreOption flashlight;

void init_flashlight();
void apply_flashlight(CthughaBuffer& buffer, const VisualFrameContext& context);

#endif
