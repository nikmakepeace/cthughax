// Palette randomization port used by Scene visual catalogs.

#ifndef CTHUGHA_SCENE_PALETTE_RANDOMIZER_H
#define CTHUGHA_SCENE_PALETTE_RANDOMIZER_H

class PaletteEntry;
class RandomSource;

/**
 * Mutates and returns random palette entries for Scene visual commands.
 *
 * The current implementation may still bridge to legacy palette storage, but
 * Scene visual catalog code depends only on this narrow typed port.
 */
class ScenePaletteRandomizer {
public:
    /** Destroys the palette randomizer interface. */
    virtual ~ScenePaletteRandomizer() { }

    /**
     * Randomizes the most recent random palette slot.
     *
     * @param randomSource Random source used for palette generation.
     * @param currentPaletteIndex Current Scene palette index. Legacy-backed
     *        implementations use this to update temporary global palette
     *        current state internally instead of relying on a wider mirror.
     * @return Mutated palette entry index, or a negative value on failure.
     */
    virtual int randomizeLast(
        RandomSource& randomSource, int currentPaletteIndex) = 0;

    /**
     * Adds a new random palette slot.
     *
     * @param randomSource Random source used for palette generation.
     * @return Added palette entry index, or a negative value on failure.
     */
    virtual int addRandom(RandomSource& randomSource) = 0;

    /**
     * Returns the random palette entry at an index.
     *
     * @param index Palette entry index.
     * @return Palette entry, or NULL when the index is invalid.
     */
    virtual PaletteEntry* paletteEntry(int index) = 0;
};

#endif
