// EffectChoice-backed Scene choice catalog adapters.

#ifndef CTHUGHA_SCENE_EFFECT_CHOICE_CATALOG_H
#define CTHUGHA_SCENE_EFFECT_CHOICE_CATALOG_H

#include "SceneChoiceSelection.h"

#include <memory>
#include <vector>

class EffectChoice;
class EffectChoiceList;
class OptionOnOff;

/**
 * SceneChoice view over one legacy EffectChoice entry.
 */
class SceneEffectChoice : public SceneChoice {
    EffectChoice& choice;

public:
    /** Creates a Scene-facing view over a borrowed legacy choice. */
    explicit SceneEffectChoice(EffectChoice& choice_);

    /** @return Borrowed legacy choice entry. */
    EffectChoice& effectChoice();

    /** @return Borrowed immutable legacy choice entry. */
    const EffectChoice& effectChoice() const;

    /** @return Choice name. */
    virtual const char* name() const;

    /** Returns nonzero when the supplied text names this choice. */
    virtual int sameName(const char* other) const;

    /** @return Nonzero when this choice may be selected randomly/cyclically. */
    virtual int inUse() const;

    /** Sets whether this choice may be selected randomly/cyclically. */
    virtual void setUse(int inUse);
};

/**
 * SceneChoiceLock view over a legacy OptionOnOff lock.
 */
class SceneEffectChoiceLock : public SceneChoiceLock {
    OptionOnOff& lockValue;

public:
    /** Creates a Scene-facing lock over a borrowed legacy option. */
    explicit SceneEffectChoiceLock(OptionOnOff& lockValue_);

    /** @return Nonzero when the lock is enabled. */
    virtual int enabled() const;

    /** Changes the lock from text such as "on" or "off". */
    virtual void change(const char* to);
};

/**
 * SceneChoiceCatalog backed by a legacy EffectChoiceList.
 */
class SceneEffectChoiceCatalog : public SceneChoiceCatalog {
    const char* optionNameValue;
    EffectChoiceList& entries;
    SceneEffectChoiceLock lockValue;
    mutable std::vector<std::unique_ptr<SceneEffectChoice> > choices;

public:
    /**
     * Creates a Scene-facing catalog over borrowed legacy choice storage.
     *
     * @param optionName_ Stable catalog/option name.
     * @param entries_ Borrowed legacy choice entries.
     * @param lockValue_ Borrowed lock state for this selection.
     */
    SceneEffectChoiceCatalog(const char* optionName_,
        EffectChoiceList& entries_, OptionOnOff& lockValue_);

    /** @return Number of choices currently in the borrowed catalog. */
    virtual int entryCount() const;

    /** @return Scene-facing choice at index, or NULL when out of range. */
    virtual SceneChoice* choiceAt(int index) const;

    /** @return Mutable lock for the selection using this catalog. */
    virtual SceneChoiceLock& lock();

    /** @return Immutable lock for the selection using this catalog. */
    virtual const SceneChoiceLock& lock() const;

    /** @return Stable catalog/option name. */
    virtual const char* optionName() const;
};

#endif
