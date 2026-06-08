/** @file
 * Visual border option and indexed-buffer mutation.
 */

#ifndef __BORDER_H
#define __BORDER_H

#include "EffectControl.h"

class FrameRenderTarget;
class FrameRenderContext;

extern EffectControl border;

/** Initializes the built-in border option catalog. */
void init_border();

/**
 * Writes hidden border rows into the active indexed buffer.
 *
 * @param buffer Buffer whose hidden rows should be updated.
 * @param context Current visual-frame audio and timing context.
 * @param borderMode Border mode selected by the border option.
 */
void apply_border(FrameRenderTarget& buffer, const FrameRenderContext& context, int borderMode);

#endif
