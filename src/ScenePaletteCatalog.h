// Native Scene catalog owner for palette entries.

#ifndef CTHUGHA_SCENE_PALETTE_CATALOG_H
#define CTHUGHA_SCENE_PALETTE_CATALOG_H

#include <memory>
#include <vector>

class PaletteEntry;

/**
 * Owns palette entries exposed to Scene selections.
 *
 * The catalog copies palette colors and metadata, so initial Scene palette
 * selections no longer need to read payloads from the temporary legacy palette
 * option.
 */
class ScenePaletteCatalog {
    class Entry {
        std::unique_ptr<PaletteEntry> paletteValue;
        int inUseValue;

    public:
        Entry(const PaletteEntry& palette_, int inUse_);
        ~Entry();

        const PaletteEntry* palette() const;
        int inUse() const;
    };

    std::vector<std::unique_ptr<Entry> > entries;

public:
    /** Creates an empty palette catalog. */
    ScenePaletteCatalog();

    /** Removes all owned entries. */
    void clear();

    /**
     * Adds one copied palette entry.
     *
     * @param palette Palette entry to copy.
     * @param inUse Nonzero when the entry is selectable by default.
     */
    void addChoice(const PaletteEntry& palette, int inUse);

    /** @return Total number of palette choices. */
    int entryCount() const;

    /**
     * @param index Catalog index.
     * @return Owned palette entry, or NULL.
     */
    const PaletteEntry* paletteAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Nonzero when the entry is selectable by default.
     */
    int inUseAt(int index) const;
};

#endif
