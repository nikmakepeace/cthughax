// Catalog-backed Scene visual selection state.

#ifndef CTHUGHA_SCENE_CHOICE_SELECTION_H
#define CTHUGHA_SCENE_CHOICE_SELECTION_H

#include "SceneVisualSelections.h"

#include <memory>
#include <vector>

class SceneChoice {
public:
    virtual ~SceneChoice();

    virtual const char* name() const = 0;
    virtual int sameName(const char* other) const = 0;
    virtual int inUse() const = 0;
    virtual void setUse(int inUse) = 0;
};

class SceneChoiceLock {
public:
    virtual ~SceneChoiceLock();

    /** @return Nonzero when random selection changes should be suppressed. */
    virtual int enabled() const = 0;

    /**
     * Changes the lock using yes/no text such as "on", "off", "1", or "0".
     */
    virtual void change(const char* to) = 0;
};

/**
 * Native owned lock state for a Scene choice selection.
 *
 * This keeps Scene selections independent from legacy option storage. Any
 * legacy controls that still need the lock bit must mirror it explicitly at a
 * synchronization boundary.
 */
class SceneChoiceLockValue : public SceneChoiceLock {
    int enabledValue;

public:
    /** Creates a lock with the given initial enabled state. */
    explicit SceneChoiceLockValue(int enabled_ = 0);

    /** @return Nonzero when random selection changes should be suppressed. */
    virtual int enabled() const;

    /** Changes the lock using yes/no text such as "on", "off", "1", or "0". */
    virtual void change(const char* to);
};

class SceneChoiceCatalog {
public:
    virtual ~SceneChoiceCatalog();

    virtual int entryCount() const = 0;
    virtual SceneChoice* choiceAt(int index) const = 0;
    virtual SceneChoiceLock& lock() = 0;
    virtual const SceneChoiceLock& lock() const = 0;
    virtual const char* optionName() const = 0;
};

class SceneChoiceSelection : public virtual SceneOptionSelection {
    std::unique_ptr<SceneChoiceCatalog> catalog;
    mutable std::vector<SceneChoice*> choices;
    mutable int catalogEntryCount;
    int selectedValue;

protected:
    SceneChoiceLock& selectionLock();
    const SceneChoiceLock& selectionLock() const;
    const char* optionName() const;
    const char* applySelectionLockPrefix(const char* to);
    void refreshCatalog() const;
    virtual SceneChoice* choiceAt(int index);
    virtual const SceneChoice* choiceAt(int index) const;
    SceneChoice* currentChoice();
    const SceneChoice* currentChoice() const;
    void setSelectedValue(int index);
    virtual void selectionChanged();

public:
    SceneChoiceSelection(SceneChoiceCatalog* catalog_, int selectedValue_);

    virtual const char* catalogName() const;
    virtual const char* currentName() const;
    virtual int currentValue() const;
    virtual int entryCount() const;
    virtual int choiceCount() const;
    virtual int resolveValue(const char* text, int* selection) const;
    virtual void change(int by);
    virtual void change(const char* to, RandomSource& randomSource);
    virtual int changeRandom(RandomSource& randomSource);
    virtual void activate(int index);
    virtual int lockEnabled() const;
    virtual void toggleLock();
    virtual void toggleChoiceUse(int index);
    virtual void setValue(int index);
};

#endif
