/** @file
 * Narrow flashlight renderer API for frame filters.
 */

#ifndef CTHUGHA_FLASHLIGHT_RENDERER_H
#define CTHUGHA_FLASHLIGHT_RENDERER_H

class FramePalette;
class FrameGeneratorContext;

/**
 * Brightens the current frame palette from acoustic fire energy.
 *
 * @param framePalette Palette state for the current displayed frame.
 * @param context Per-frame audio/acoustic context.
 */
void apply_flashlight(FramePalette& framePalette,
    const FrameGeneratorContext& context);

#endif
