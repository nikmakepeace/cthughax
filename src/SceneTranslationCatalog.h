// Native Scene catalog owner for generated translation tables.

#ifndef CTHUGHA_SCENE_TRANSLATION_CATALOG_H
#define CTHUGHA_SCENE_TRANSLATION_CATALOG_H

#include "TranslationTable.h"

#include <string>
#include <vector>

class RandomSource;

/**
 * Owns the translation-table entries exposed to Scene selections.
 *
 * The catalog always contains the built-in "none" entry at index zero. When
 * translation effects are enabled, load() appends generated translation tables
 * using the same built-in generator catalog that historically populated the
 * temporary legacy translation option.
 */
class SceneTranslationCatalog {
    class Entry {
    public:
        std::string name;
        std::string description;
        std::vector<int> table;
        int width;
        int height;
        int inUse;

        Entry();
        Entry(const char* name_, const char* description_,
            const int* data_, int size_, int width_, int height_, int inUse_);
    };

    std::vector<Entry> entries;

public:
    /** Creates a catalog containing only the "none" entry. */
    SceneTranslationCatalog();

    /**
     * Rebuilds the owned catalog for the requested frame size.
     *
     * @param width Target frame width in pixels.
     * @param height Target frame height in pixels.
     * @param translationsEnabled Nonzero to generate built-in translation
     *        tables after the "none" entry.
     * @param randomSource Seed source for generators with randomized presets.
     */
    void load(int width, int height, int translationsEnabled,
        RandomSource& randomSource);

    /** @return Total number of Scene translation choices. */
    int entryCount() const;

    /** @return Number of generated entries after the "none" entry. */
    int generatedCount() const;

    /**
     * @param index Catalog index.
     * @return Translation table view for index, or an empty "none" table.
     */
    TranslationTable tableAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Human-readable description for index, or an empty string.
     */
    const char* descriptionAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Nonzero when the entry is selectable by default.
     */
    int inUseAt(int index) const;

};

#endif
