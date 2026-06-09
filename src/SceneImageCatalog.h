// Native Scene catalog owner for indexed-image entries.

#ifndef CTHUGHA_SCENE_IMAGE_CATALOG_H
#define CTHUGHA_SCENE_IMAGE_CATALOG_H

#include <memory>
#include <string>
#include <vector>

class IndexedImage;

/**
 * Owns indexed-image entries exposed to Scene selections.
 *
 * The catalog copies image pixels and palette data, so Scene selections no
 * longer need to read image payloads from the temporary legacy image option.
 */
class SceneImageCatalog {
    class Entry {
        std::string nameValue;
        std::unique_ptr<IndexedImage> imageValue;
        int inUseValue;

    public:
        Entry(const char* name_, const IndexedImage* image_, int inUse_);
        ~Entry();

        const char* name() const;
        const IndexedImage* image() const;
        int inUse() const;
    };

    std::vector<std::unique_ptr<Entry> > entries;

public:
    /** Creates an empty image catalog. */
    SceneImageCatalog();

    /** Removes all owned entries. */
    void clear();

    /**
     * Adds one copied image entry.
     *
     * @param name Stable selection name.
     * @param image Optional indexed image to copy.
     * @param inUse Nonzero when the entry is selectable by default.
     */
    void addChoice(const char* name, const IndexedImage* image, int inUse);

    /** @return Total number of image choices. */
    int entryCount() const;

    /**
     * @param index Catalog index.
     * @return Selection name for index, or an empty string.
     */
    const char* nameAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Owned indexed image, or NULL.
     */
    const IndexedImage* imageAt(int index) const;

    /**
     * @param index Catalog index.
     * @return Nonzero when the entry is selectable by default.
     */
    int inUseAt(int index) const;
};

#endif
