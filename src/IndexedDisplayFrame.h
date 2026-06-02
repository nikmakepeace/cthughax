#ifndef __INDEXED_DISPLAY_FRAME_H
#define __INDEXED_DISPLAY_FRAME_H

class FramePalette;

/**
 * Indexed pixels produced by the selected screen/presentation effect.
 *
 * This is display-agnostic Cthugha data: one byte per pixel plus a palette.
 * Backend-native surfaces, textures, XImages, and Wayland buffers are separate
 * objects that consume this frame later.
 */
class IndexedDisplayFrame {
    unsigned char* pixelsValue;
    int widthValue;
    int heightValue;
    int pitchValue;
    int capacityByteCountValue;
    FramePalette* framePaletteValue;

public:
    IndexedDisplayFrame();
    ~IndexedDisplayFrame();

    IndexedDisplayFrame(const IndexedDisplayFrame&) = delete;
    IndexedDisplayFrame& operator=(const IndexedDisplayFrame&) = delete;

    /**
     * Clears owned pixels and returns the frame to an invalid empty state.
     */
    void reset();

    /**
     * Resizes to tightly packed indexed rows.
     *
     * @param width Visible pixels per row.
     * @param height Visible rows.
     */
    void resize(int width, int height);

    /**
     * Resizes to indexed rows with an explicit pitch.
     *
     * @param width Visible pixels per row.
     * @param height Visible rows.
     * @param pitch Bytes from the start of one row to the start of the next.
     */
    void resize(int width, int height, int pitch);

    int valid() const;

    unsigned char* pixels() { return pixelsValue; }
    const unsigned char* pixels() const { return pixelsValue; }

    unsigned char* line(int y) { return pixelsValue + y * pitchValue; }
    const unsigned char* line(int y) const { return pixelsValue + y * pitchValue; }

    int width() const { return widthValue; }
    int height() const { return heightValue; }
    int pitch() const { return pitchValue; }
    int byteCount() const { return valid() ? pitchValue * heightValue : 0; }
    int capacityByteCount() const { return capacityByteCountValue; }

    FramePalette* framePalette() const { return framePaletteValue; }
    void setFramePalette(FramePalette* framePalette) { framePaletteValue = framePalette; }
};

#endif
