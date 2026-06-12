/** @file
 * Narrow border renderer API for frame filters.
 */

#ifndef CTHUGHA_BORDER_RENDERER_H
#define CTHUGHA_BORDER_RENDERER_H

class FrameStageBuffer;
class FrameGeneratorContext;

/**
 * Writes hidden border rows into the destination indexed buffer.
 *
 * @param buffer Buffer whose hidden rows should be updated.
 * @param context Current visual-frame audio and timing context.
 * @param borderMode Selected border drawing mode.
 */
void apply_border(FrameStageBuffer& buffer,
    const FrameGeneratorContext& context, int borderMode);

#endif
