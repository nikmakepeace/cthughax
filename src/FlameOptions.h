// Legacy flame option declarations.

#ifndef CTHUGHA_FLAME_OPTIONS_H
#define CTHUGHA_FLAME_OPTIONS_H

#include "EffectControl.h"

class Flame;
class RandomSource;

/**
 * Effect choice that points at one built-in Flame catalog item.
 */
class FlameEntry : public EffectChoice {
    const Flame* flameValue;

public:
    /**
     * Creates an option entry for a flame.
     *
     * @param flame Borrowed flame catalog entry.
     * @param inUse Nonzero when the option should be selectable by default.
     */
    FlameEntry(const Flame& flame, int inUse = 1);

    /** @return Borrowed flame catalog entry. */
    const Flame& flame() const;
};

/**
 * Option wrapper for selecting the current flame feedback kernel.
 */
class FlameOption : public EffectControl {
public:
    FlameOption();

    /** @return Currently selected flame, or NULL. */
    const Flame* currentFlame();
};

/** Global flame selection option. */
extern FlameOption flame;

/**
 * Option for encoded general-flame neighbor choices.
 *
 * The value is a base-9 encoding consumed by GenSubt/GenSlow flame kernels.
 * Text changes may also carry lock prefixes such as "lock:" or "no-lock:".
 */
class GeneralFlameOption : public EffectControl {
public:
    GeneralFlameOption();

    /**
     * Changes the general-flame value from text.
     *
     * @param to Decimal value, optionally prefixed by a lock directive.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(const char* to, int doSave = 1);

    /**
     * Changes the general-flame value from text using an injected random source
     * for invalid/empty random fallback cases.
     *
     * @param to Decimal value, optionally prefixed by a lock directive.
     * @param randomSource Random source used for fallback selection.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(const char* to, RandomSource& randomSource, int doSave = 1);

    /**
     * Changes the encoded value by a relative amount.
     *
     * @param by Delta applied modulo the number of encoded states.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void change(int by, int doSave = 1);

    /**
     * Picks a random encoded general-flame value unless locked.
     *
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void changeRandom(int doSave = 1);

    /**
     * Picks a random encoded general-flame value from an injected source unless
     * locked.
     *
     * @param randomSource Random source used to select the encoded value.
     * @param doSave Nonzero to save the previous option value first.
     */
    virtual void changeRandom(RandomSource& randomSource, int doSave = 1);

    /** @return Current value text, including "locked:" when locked. */
    virtual const char* text() const;
};

/** Global encoded general-flame option. */
extern GeneralFlameOption flameGeneral;

/** Legacy flame option entry array used by initialization/UI code. */
extern EffectChoice* _flames[];

/** Number of entries in _flames. */
extern int _nFlames;

#endif
