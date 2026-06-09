// Typed Scene visual choice catalogs.

#ifndef CTHUGHA_SCENE_TYPED_VISUAL_CATALOGS_H
#define CTHUGHA_SCENE_TYPED_VISUAL_CATALOGS_H

#include "SceneChoiceSelection.h"

#include <memory>
#include <string>
#include <vector>

class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;

/**
 * Owned SceneChoice entry that points directly at a Flame catalog item.
 */
class SceneFlameChoice : public SceneChoice {
    const Flame* flameValue;
    std::string nameValue;
    int inUseValue;

public:
    /**
     * Creates one flame choice.
     *
     * @param flame_ Borrowed flame catalog item returned by currentFlame().
     * @param name_ Stable choice name.
     * @param inUse_ Nonzero when selectable by default.
     */
    SceneFlameChoice(const Flame* flame_, const char* name_, int inUse_);

    /** @return Borrowed flame catalog item. */
    const Flame* flame() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for built-in Flame choices.
 */
class SceneFlameChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneFlameChoice> > choices;

public:
    /**
     * Creates an empty flame choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneFlameChoiceCatalog(const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one flame choice.
     *
     * @param flame Borrowed flame catalog item returned by the selection.
     * @param name Stable choice name.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    SceneFlameChoice& addChoice(
        const Flame* flame, const char* name, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed flame selection that returns a typed Flame pointer.
 */
class SceneFlameChoiceSelection : public SceneChoiceSelection,
    public SceneFlameSelection {
public:
    /**
     * Creates a flame selection over an owned catalog.
     *
     * @param catalog Owned flame catalog.
     * @param selectedValue Initial selected index.
     */
    SceneFlameChoiceSelection(SceneChoiceCatalog* catalog, int selectedValue);

    /** @return Selected flame catalog item, or NULL. */
    virtual const Flame* currentFlame();
};

/**
 * Owned SceneChoice entry that points directly at a Wave catalog item.
 */
class SceneWaveChoice : public SceneChoice {
    Wave* waveValue;
    std::string nameValue;
    int inUseValue;

public:
    /**
     * Creates one wave choice.
     *
     * @param wave_ Borrowed wave catalog item returned by currentWave().
     * @param name_ Stable choice name.
     * @param inUse_ Nonzero when selectable by default.
     */
    SceneWaveChoice(Wave* wave_, const char* name_, int inUse_);

    /** @return Borrowed wave catalog item. */
    Wave* wave() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for built-in Wave choices.
 */
class SceneWaveChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneWaveChoice> > choices;

public:
    /**
     * Creates an empty wave choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneWaveChoiceCatalog(const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one wave choice.
     *
     * @param wave Borrowed wave catalog item returned by the selection.
     * @param name Stable choice name.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    SceneWaveChoice& addChoice(Wave* wave, const char* name, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed wave selection that returns a typed Wave pointer.
 */
class SceneWaveChoiceSelection : public SceneChoiceSelection,
    public SceneWaveSelection {
public:
    /**
     * Creates a wave selection over an owned catalog.
     *
     * @param catalog Owned wave catalog.
     * @param selectedValue Initial selected index.
     */
    SceneWaveChoiceSelection(SceneChoiceCatalog* catalog, int selectedValue);

    /** @return Selected wave catalog item, or NULL. */
    virtual Wave* currentWave();
};

/**
 * Owned SceneChoice entry that stores a copied wave object payload.
 */
class SceneWaveObjectChoice : public SceneChoice {
    std::unique_ptr<WObject[]> objectValue;
    std::string nameValue;
    int inUseValue;

public:
    /**
     * Creates one wave-object choice.
     *
     * @param name_ Stable choice name.
     * @param object_ Optional terminated line list copied into the choice.
     * @param inUse_ Nonzero when selectable by default.
     */
    SceneWaveObjectChoice(const char* name_, const WObject* object_,
        int inUse_);

    /** @return Mutable owned terminated line list, or NULL. */
    WObject* object() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for wave-object choices.
 */
class SceneWaveObjectChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneWaveObjectChoice> > choices;

public:
    /**
     * Creates an empty wave-object choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneWaveObjectChoiceCatalog(
        const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one copied wave-object choice.
     *
     * @param name Stable choice name.
     * @param object Optional terminated line list copied into this catalog.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    SceneWaveObjectChoice& addChoice(
        const char* name, const WObject* object, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed object selection that returns an owned WObject line list.
 */
class SceneWaveObjectChoiceSelection : public SceneChoiceSelection,
    public SceneWaveObjectSelection {
public:
    /**
     * Creates a wave-object selection over an owned catalog.
     *
     * @param catalog Owned wave-object catalog.
     * @param selectedValue Initial selected index.
     */
    SceneWaveObjectChoiceSelection(
        SceneChoiceCatalog* catalog, int selectedValue);

    /** @return Selected owned object line list, or NULL. */
    virtual WObject* currentObject();
};

/**
 * Owned SceneChoice entry that stores one TranslationTable payload.
 */
class SceneTranslationChoice : public SceneChoice {
    std::string nameValue;
    std::vector<int> tableData;
    TranslationTable tableValue;
    int inUseValue;

public:
    /**
     * Creates one translation-table choice.
     *
     * @param table_ Translation table to copy into the Scene choice.
     * @param inUse_ Nonzero when selectable by default.
     */
    SceneTranslationChoice(const TranslationTable& table_, int inUse_);

    /** @return Immutable table view backed by this owned choice. */
    TranslationTable table() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for translation-table choices.
 */
class SceneTranslationChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneTranslationChoice> > choices;

public:
    /**
     * Creates an empty translation-table choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneTranslationChoiceCatalog(
        const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one translation-table choice.
     *
     * @param table Translation table copied into this catalog.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    SceneTranslationChoice& addChoice(
        const TranslationTable& table, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed translation selection that returns an owned table view.
 */
class SceneTranslationChoiceSelection : public SceneChoiceSelection,
    public SceneTranslationSelection {
public:
    /**
     * Creates a translation-table selection over an owned catalog.
     *
     * @param catalog Owned translation-table catalog.
     * @param selectedValue Initial selected index.
     */
    SceneTranslationChoiceSelection(
        SceneChoiceCatalog* catalog, int selectedValue);

    /** @return Selected translation table, or an empty table. */
    virtual TranslationTable currentTranslationTable();
};

/**
 * Owned SceneChoice entry that stores a copied PaletteEntry payload.
 */
class ScenePaletteChoice : public SceneChoice {
    std::unique_ptr<PaletteEntry> paletteValue;
    std::string nameValue;
    int inUseValue;

public:
    /**
     * Creates one palette choice.
     *
     * @param palette_ Palette entry copied into the Scene choice.
     * @param inUse_ Nonzero when selectable by default.
     */
    ScenePaletteChoice(const PaletteEntry& palette_, int inUse_);
    ~ScenePaletteChoice();

    /**
     * Replaces the owned payload without changing this choice object's
     * identity.
     *
     * @param palette_ Palette entry copied into this choice.
     * @param inUse_ Nonzero when selectable by default.
     */
    void copyFrom(const PaletteEntry& palette_, int inUse_);

    /** @return Mutable owned palette entry. */
    PaletteEntry* paletteEntry() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for palette choices.
 */
class ScenePaletteChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<ScenePaletteChoice> > choices;

public:
    /**
     * Creates an empty palette choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    ScenePaletteChoiceCatalog(const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one copied palette choice.
     *
     * @param palette Palette entry copied into this catalog.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    ScenePaletteChoice& addChoice(const PaletteEntry& palette, int inUse);

    /**
     * Replaces a copied palette choice in place.
     *
     * @param index Choice index to replace.
     * @param palette Palette entry copied into this catalog.
     * @param inUse Nonzero when selectable by default.
     * @return Nonzero when the index existed and was replaced.
     */
    int replaceChoice(int index, const PaletteEntry& palette, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed palette selection that returns an owned PaletteEntry.
 */
class ScenePaletteChoiceSelection : public SceneChoiceSelection,
    public ScenePaletteSelection {
    ScenePaletteChoiceCatalog* paletteCatalogValue;

public:
    /**
     * Creates a palette selection over an owned catalog.
     *
     * @param catalog Owned palette catalog.
     * @param selectedValue Initial selected index.
     */
    ScenePaletteChoiceSelection(SceneChoiceCatalog* catalog, int selectedValue);

    /**
     * Appends a copied palette entry to the owned catalog.
     *
     * @param palette Palette entry copied into this selection.
     * @param inUse Nonzero when selectable by default.
     * @return New entry index, or -1 when the catalog is unavailable.
     */
    int appendPaletteEntry(const PaletteEntry& palette, int inUse);

    /**
     * Replaces a copied palette entry in the owned catalog.
     *
     * @param index Existing entry index.
     * @param palette Palette entry copied into this selection.
     * @param inUse Nonzero when selectable by default.
     * @return Nonzero when the entry was replaced.
     */
    int replacePaletteEntry(int index, const PaletteEntry& palette, int inUse);

    /** @return Selected owned palette entry, or NULL. */
    virtual PaletteEntry* currentPaletteEntry();
};

/**
 * Owned SceneChoice entry that stores a copied IndexedImage payload.
 */
class SceneImageChoice : public SceneChoice {
    std::unique_ptr<IndexedImage> imageValue;
    std::string nameValue;
    int inUseValue;

public:
    /**
     * Creates one image choice.
     *
     * @param name_ Stable choice name.
     * @param image_ Optional image copied into the Scene choice.
     * @param inUse_ Nonzero when selectable by default.
     */
    SceneImageChoice(const char* name_, const IndexedImage* image_,
        int inUse_);
    ~SceneImageChoice();

    /** @return Immutable owned image, or NULL. */
    const IndexedImage* image() const;

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * Owned SceneChoiceCatalog for image choices.
 */
class SceneImageChoiceCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneImageChoice> > choices;

public:
    /**
     * Creates an empty image choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneImageChoiceCatalog(const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one copied image choice.
     *
     * @param name Stable choice name.
     * @param image Optional image copied into this catalog.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry.
     */
    SceneImageChoice& addChoice(
        const char* name, const IndexedImage* image, int inUse);

    /** @return Number of owned choices. */
    virtual int entryCount() const;

    /** @return Choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

/**
 * Catalog-backed image selection that returns an owned IndexedImage.
 */
class SceneImageChoiceSelection : public SceneChoiceSelection,
    public SceneImageSelection {
public:
    /**
     * Creates an image selection over an owned catalog.
     *
     * @param catalog Owned image catalog.
     * @param selectedValue Initial selected index.
     */
    SceneImageChoiceSelection(SceneChoiceCatalog* catalog, int selectedValue);

    /** @return Selected owned image, or NULL. */
    virtual const IndexedImage* currentImage();
};

/**
 * Creates a wave-object choice catalog from native Scene wave-object entries.
 *
 * The returned catalog owns its choices and the supplied lock.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @param waveObjects Native wave-object catalog to expose.
 * @return Owned catalog with copied wave-object choices.
 */
SceneChoiceCatalog* createSceneWaveObjectChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock,
    const SceneWaveObjectCatalog& waveObjects);

/**
 * Creates a translation choice catalog from native Scene translation entries.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @param translations Native translation catalog to expose.
 * @return Owned catalog with copied translation choices.
 */
SceneChoiceCatalog* createSceneTranslationChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock,
    const SceneTranslationCatalog& translations);

/**
 * Creates a palette choice catalog from native Scene palette entries.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @param palettes Native palette catalog to expose.
 * @return Owned catalog with copied palette choices.
 */
SceneChoiceCatalog* createScenePaletteChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock,
    const ScenePaletteCatalog& palettes);

/**
 * Creates an image choice catalog from native Scene image entries.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @param images Native image catalog to expose.
 * @return Owned catalog with copied image choices.
 */
SceneChoiceCatalog* createSceneImageChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock,
    const SceneImageCatalog& images);

#endif
