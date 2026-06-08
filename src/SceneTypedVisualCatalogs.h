// Typed Scene visual choice catalogs.

#ifndef CTHUGHA_SCENE_TYPED_VISUAL_CATALOGS_H
#define CTHUGHA_SCENE_TYPED_VISUAL_CATALOGS_H

#include "SceneChoiceSelection.h"

#include <memory>
#include <string>
#include <vector>

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

#endif
