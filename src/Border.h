// Visual border option and indexed-buffer mutation.

#ifndef __BORDER_H
#define __BORDER_H

#include "CoreOption.h"

class CthughaBuffer;
class VisualFrameContext;

extern CoreOption border;

void init_border();
void apply_border(CthughaBuffer& buffer, const VisualFrameContext& context, int borderMode);

#endif
