// Native encoded general-flame Scene selection.

#ifndef CTHUGHA_SCENE_GENERAL_FLAME_SELECTION_VALUE_H
#define CTHUGHA_SCENE_GENERAL_FLAME_SELECTION_VALUE_H

#include "SceneChoiceSelection.h"

#include <memory>
#include <string>

class LogSink;

/**
 * Scene-owned selection value for the encoded general-flame state.
 *
 * General flame is not a finite EffectChoice catalog. It is a base-9 encoded
 * integer consumed by the general flame kernels, plus a lock. This class keeps
 * that state behind the normal SceneGeneralFlameSelection port without knowing
 * about legacy option storage.
 */
class SceneGeneralFlameSelectionValue : public SceneGeneralFlameSelection {
    std::string catalogNameValue;
    std::unique_ptr<SceneChoiceLock> lockValue;
    int selectedValue;
    LogSink* logValue;
    mutable char selectionTextValue[32];

    const char* applyLockPrefix(const char* text);

public:
    /**
     * Creates an encoded general-flame selection.
     *
     * @param catalogName_ Stable selection/catalog name.
     * @param lock_ Owned lock state for this selection.
     * @param selectedValue_ Initial encoded value.
     * @param log_ Optional diagnostics sink for invalid text.
     */
    SceneGeneralFlameSelectionValue(const char* catalogName_,
        SceneChoiceLock* lock_, int selectedValue_, LogSink* log_ = 0);

    /** @return Stable selection/catalog name. */
    virtual const char* catalogName() const;

    /** @return Current selection text, including "locked:" when locked. */
    virtual const char* currentName() const;

    /** @return Current encoded value. */
    virtual int currentValue() const;

    /** @return Number of legal encoded values. */
    virtual int entryCount() const;

    /** Changes the encoded value by a relative amount. */
    virtual void change(int by);

    /**
     * Changes the encoded value from decimal text.
     *
     * Text may carry the same lock prefixes as catalog-backed selections, such
     * as "locked:" or "no-lock:".
     */
    virtual void change(const char* to, RandomSource& randomSource);

    /** Picks a random encoded value unless locked. */
    virtual int changeRandom(RandomSource& randomSource);

    /** Activates the encoded value when the index is in range. */
    virtual void activate(int index);

    /** @return Nonzero when random selection changes should be suppressed. */
    virtual int lockEnabled() const;

    /** Toggles the selection lock. */
    virtual void toggleLock();

    /** Sets the encoded value modulo the legal range. */
    virtual void setValue(int index);

    /** @return Current encoded value. */
    virtual int encodedValue() const;

    /** @return Current selection text, including "locked:" when locked. */
    virtual const char* selectionText() const;
};

#endif
