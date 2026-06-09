// Runtime translation executor.

#ifndef __TRANSLATE_RUNTIME_H
#define __TRANSLATE_RUNTIME_H

#include "TranslationTable.h"

class FrameRenderTarget;
class FrameGeneratorContext;

/**
 * Runtime executor for one translation table.
 *
 * Translate swaps active/passive buffers, then remaps passive pixels into the
 * active buffer using TranslationTable source-pixel indexes.
 */
class Translate {
    TranslationTable tableValue;

public:
    /** Creates an empty executor that does nothing until given a table. */
    Translate();

    /**
     * Creates an executor for a generated/loaded translation table.
     *
     * @param table Borrowed immutable table view.
     */
    explicit Translate(const TranslationTable& table);

    /** @return Translation table display name. */
    const char* name() const;

    /** @return Nonzero when the table has mapping data. */
    int ready() const;

    /**
     * Applies the coordinate remap to a frame buffer.
     *
     * @param buffer Active/passive indexed pixel buffer. Dimensions must match
     *        the translation table.
     * @param context Current frame render context; currently unused by translate.
     */
    void execute(FrameRenderTarget& buffer, const FrameGeneratorContext& context) const;
};

#endif
