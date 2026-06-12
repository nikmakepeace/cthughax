// Immutable view over a loaded translation table.

#ifndef __TRANSLATION_TABLE_H
#define __TRANSLATION_TABLE_H

/**
 * Borrowed view over a coordinate-remap table.
 *
 * Each entry is a source pixel index into the frame source. Entry N tells
 * Translate which source pixel should become destination pixel N.
 */
class TranslationTable {
    const char* nameValue;
    const int* dataValue;
    int widthValue;
    int heightValue;
    int sizeValue;

public:
    /** Creates an empty "none" table. */
    TranslationTable()
        : nameValue("none")
        , dataValue(0)
        , widthValue(0)
        , heightValue(0)
        , sizeValue(0) { }

    /**
     * Wraps an existing translation table without taking ownership.
     *
     * @param name_ Display/option name for diagnostics.
     * @param data_ Source-pixel indexes, length width_ * height_, or NULL.
     * @param width_ Table width in pixels.
     * @param height_ Table height in pixels.
     */
    TranslationTable(const char* name_, const int* data_, int width_, int height_)
        : nameValue((name_ != 0) ? name_ : "unknown")
        , dataValue(data_)
        , widthValue(width_)
        , heightValue(height_)
        , sizeValue(width_ * height_) { }

    /** @return Display/option name for this table. */
    const char* name() const { return nameValue; }

    /** @return Source-pixel index array, or NULL for an empty table. */
    const int* data() const { return dataValue; }

    /** @return Width in pixels. */
    int width() const { return widthValue; }

    /** @return Height in pixels. */
    int height() const { return heightValue; }

    /** @return Number of pixel-index entries. */
    int size() const { return sizeValue; }

    /** @return Nonzero when data() contains at least one pixel mapping. */
    int ready() const { return dataValue != 0 && sizeValue > 0; }

    /**
     * Compares table identity and dimensions.
     *
     * @param other Table view to compare with.
     * @return Nonzero when both views point to the same mapping data and size.
     */
    int sameTable(const TranslationTable& other) const {
        return dataValue == other.dataValue
            && widthValue == other.widthValue
            && heightValue == other.heightValue
            && sizeValue == other.sizeValue;
    }
};

#endif
