// Owned Scene choice catalog.

#ifndef CTHUGHA_SCENE_CHOICE_LIST_CATALOG_H
#define CTHUGHA_SCENE_CHOICE_LIST_CATALOG_H

#include "SceneChoiceSelection.h"

#include <memory>
#include <string>
#include <vector>

/**
 * Owned SceneChoice implementation with optional text aliases.
 */
class SceneChoiceListEntry : public SceneChoice {
    std::string nameValue;
    std::vector<std::string> aliases;
    int inUseValue;

public:
    /**
     * Creates one owned Scene choice.
     *
     * @param name_ Stable choice name.
     * @param inUse_ Nonzero when the choice is selectable by default.
     */
    SceneChoiceListEntry(const char* name_, int inUse_);

    /** Adds an alternate text token that resolves to this choice. */
    void addAlias(const char* alias);

    /** @return Stable choice name. */
    virtual const char* name() const;

    /** Returns nonzero when text names this choice or one of its aliases. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * SceneChoiceCatalog backed by owned choice entries and lock state.
 */
class SceneChoiceListCatalog : public SceneChoiceCatalog {
    std::string optionNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    std::vector<std::unique_ptr<SceneChoiceListEntry> > choices;

public:
    /**
     * Creates an empty owned choice catalog.
     *
     * @param optionName_ Stable catalog/option name.
     * @param lock_ Owned lock state for this selection.
     */
    SceneChoiceListCatalog(const char* optionName_, SceneChoiceLock* lock_);

    /**
     * Adds one owned choice.
     *
     * @param name Stable choice name.
     * @param inUse Nonzero when selectable by default.
     * @return Mutable choice entry for adding aliases.
     */
    SceneChoiceListEntry& addChoice(const char* name, int inUse);

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

#endif
