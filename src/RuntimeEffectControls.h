/** @file
 * Runtime EffectControl state port and default implementation.
 */

#ifndef CTHUGHA_RUNTIME_EFFECT_CONTROLS_H
#define CTHUGHA_RUNTIME_EFFECT_CONTROLS_H

#include "RuntimeCommandSink.h"

class EffectControl;
class RandomSource;

/** Controls runtime commands for legacy EffectControl state. */
class RuntimeEffectControls {
public:
    /** Destroys the EffectControl state interface. */
    virtual ~RuntimeEffectControls() { }

    /**
     * Changes an effect control by relative offset.
     *
     * @param control Effect control to change.
     * @param by Relative offset to apply.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl& control, int by) = 0;

    /**
     * Changes an effect control by choice text.
     *
     * @param control Effect control to change.
     * @param to Choice text to select.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl& control, const char* to) = 0;

    /**
     * Activates an effect-control choice by index.
     *
     * @param control Effect control to change.
     * @param index Choice index to activate.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet activateEffectControl(
        EffectControl& control, int index) = 0;

    /**
     * Toggles an effect control's individual lock.
     *
     * @param control Effect control whose lock should toggle.
     */
    virtual void toggleEffectControlLock(EffectControl& control) = 0;

    /**
     * Toggles whether an effect-control choice may be selected.
     *
     * @param control Effect control containing the choice.
     * @param index Choice index whose use flag should toggle.
     */
    virtual void toggleEffectChoiceUse(EffectControl& control, int index) = 0;
};

/**
 * RuntimeEffectControls implementation backed by current EffectControl APIs.
 *
 * This adapter centralizes legacy selection, per-control lock, and per-choice
 * use-flag mutations that belong to EffectControl itself rather than display,
 * audio, auto-change, or scene state.
 */
class DefaultRuntimeEffectControls : public RuntimeEffectControls {
    RandomSource& randomSource;

public:
    /**
     * Creates an adapter that uses the supplied random source for text changes
     * that request or fall back to random selection.
     *
     * @param randomSource Random source owned by the application lifecycle.
     */
    explicit DefaultRuntimeEffectControls(RandomSource& randomSource);

    /**
     * Changes an effect control by relative offset.
     *
     * @param control Effect control to change.
     * @param by Relative offset to apply.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl& control, int by);

    /**
     * Changes an effect control by choice text.
     *
     * @param control Effect control to change.
     * @param to Choice text to select.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl& control, const char* to);

    /**
     * Activates an effect-control choice by index.
     *
     * @param control Effect control to change.
     * @param index Choice index to activate.
     * @return Change flags produced by the option mutation.
     */
    virtual RuntimeChangeSet activateEffectControl(
        EffectControl& control, int index);

    /**
     * Toggles an effect control's individual lock.
     *
     * @param control Effect control whose lock should toggle.
     */
    virtual void toggleEffectControlLock(EffectControl& control);

    /**
     * Toggles whether an effect-control choice may be selected.
     *
     * @param control Effect control containing the choice.
     * @param index Choice index whose use flag should toggle.
     */
    virtual void toggleEffectChoiceUse(EffectControl& control, int index);
};

#endif
