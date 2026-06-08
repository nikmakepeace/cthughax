// Native random-palette mutation for Scene visual catalogs.

#ifndef CTHUGHA_SCENE_PALETTE_RANDOMIZER_H
#define CTHUGHA_SCENE_PALETTE_RANDOMIZER_H

class RandomSource;
class ScenePaletteChoiceSelection;

/**
 * Mutates Scene-owned random palette entries.
 *
 * This service owns the random.N slot bookkeeping for Scene palette selections
 * and persists generated entries to the legacy-compatible resources/map files.
 * It does not read or mutate the global palette option catalog.
 */
class ScenePaletteRandomizer {
    int lastRandomIndexValue;
    int lastRandomSelectionIndexValue;

public:
    /** Creates a randomizer with no previously generated random slot. */
    ScenePaletteRandomizer();

    /**
     * Randomizes the selected random.N entry or the last generated random slot.
     *
     * @param selection Scene-owned palette selection to mutate.
     * @param randomSource Random source used for palette generation.
     * @return Mutated Scene palette selection index, or a negative value when
     *         the selection could not be updated.
     */
    int randomizeLast(
        ScenePaletteChoiceSelection& selection, RandomSource& randomSource);

    /**
     * Adds a new Scene-owned random palette entry.
     *
     * @param selection Scene-owned palette selection to append to.
     * @param randomSource Random source used for palette generation.
     * @return Added Scene palette selection index, or a negative value when the
     *         selection could not be updated.
     */
    int addRandom(
        ScenePaletteChoiceSelection& selection, RandomSource& randomSource);

    /** @return Most recent random.N suffix, or -1 before one is generated. */
    int lastRandomIndex() const;

    /** @return Most recent Scene palette selection index, or -1 before use. */
    int lastRandomSelectionIndex() const;
};

#endif
