// Native Scene catalog owner for wave-object line lists.

#ifndef CTHUGHA_SCENE_WAVE_OBJECT_CATALOG_H
#define CTHUGHA_SCENE_WAVE_OBJECT_CATALOG_H

#include "Wave.h"

#include <memory>
#include <string>
#include <vector>

/**
 * Owns wave-object entries exposed to Scene selections.
 *
 * The catalog copies each terminated WObject line list that is added to it, so
 * Scene choices no longer need to read selected object payloads from temporary
 * legacy object entries.
 */
class SceneWaveObjectCatalog {
    class Entry {
        std::string nameValue;
        std::unique_ptr<WObject[]> objectValue;
        int inUseValue;

    public:
        Entry(const char* name_, const WObject* object_, int inUse_);

        const char* name() const;
        const WObject* object() const;
        int inUse() const;
    };

    std::vector<std::unique_ptr<Entry> > entries;

public:
    /** Creates an empty wave-object catalog. */
    SceneWaveObjectCatalog();

    /** Removes all owned entries. */
    void clear();

    /**
     * Adds one copied wave-object entry.
     *
     * @param name Stable selection name.
     * @param object Optional terminated WObject line list to copy.
     * @param inUse Nonzero when the entry is selectable by default.
     */
    void addChoice(const char* name, const WObject* object, int inUse);

    /** @return Total number of wave-object choices. */
    int entryCount() const;

    /**
     * @param index Catalog index.
     * @return Selection name for index, or an empty string.
     */
    const char* nameAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Owned terminated WObject line list, or NULL.
     */
    const WObject* objectAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Nonzero when the entry is selectable by default.
     */
    int inUseAt(int index) const;
};

#endif
