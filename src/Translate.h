// Runtime translation executor.

#ifndef __TRANSLATE_RUNTIME_H
#define __TRANSLATE_RUNTIME_H

#include "TranslationTable.h"

class FrameStageBuffer;
class FrameGeneratorContext;

/**
 * Runtime executor for one translation table.
 *
 * Translate remaps immutable source pixels into mutable destination pixels
 * using TranslationTable source-pixel indexes.  Physical buffer role changes
 * are owned by FrameFilterchain.
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
     * @param buffer Indexed source/destination buffer. Dimensions must match
     *        the translation table.
     * @param context Current frame render context; currently unused by translate.
     */
    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context) const;
};

#endif
