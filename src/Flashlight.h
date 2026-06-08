// Visual flashlight option.

#ifndef __FLASHLIGHT_H
#define __FLASHLIGHT_H

#include "EffectControl.h"

class FrameRenderTarget;
class FramePalette;
class FrameRenderContext;

/** Global on/off option for palette flashlight. */
extern EffectControl flashlight;

/**
 * Registers flashlight option entries.
 */
void init_flashlight();

/**
 * Brightens the current frame palette from acoustic fire energy.
 *
 * This does not touch indexed pixels. It copies the current palette, boosts
 * low palette indexes according to FrameRenderContext::acousticContext, and
 * writes the adjusted palette back to the frame palette.
 *
 * @param framePalette Palette state for the current displayed frame.
 * @param context Per-frame audio/acoustic context.
 */
void apply_flashlight(FramePalette& framePalette, const FrameRenderContext& context);

#endif
