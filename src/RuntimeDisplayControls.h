/** @file
 * Runtime display/presentation control port and default implementation.
 */

#ifndef CTHUGHA_RUNTIME_DISPLAY_CONTROLS_H
#define CTHUGHA_RUNTIME_DISPLAY_CONTROLS_H

#include "RuntimeCommandSink.h"

class EffectControl;
class DisplayPresentationSettings;
class Option;
class RandomSource;

/** Controls runtime display and presentation options. */
class RuntimeDisplayControls {
public:
    /** Destroys the display controls interface. */
    virtual ~RuntimeDisplayControls() { }

    /**
     * Changes the selected screen/presentation by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changePresentationBy(int by) = 0;

    /**
     * Changes the selected screen/presentation by name.
     *
     * @param to Choice text to select.
     */
    virtual void changePresentationTo(const char* to) = 0;

    /**
     * Changes the zoom option by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changeZoomBy(int by) = 0;

    /**
     * Changes the zoom option by name/value text.
     *
     * @param to Value text to select.
     */
    virtual void changeZoomTo(const char* to) = 0;

    /**
     * Changes the max-FPS option by absolute integer value.
     *
     * @param to Max frames per second; 0 disables pacing.
     */
    virtual void changeMaxFpsTo(int to) = 0;

    /** Toggles the FPS overlay. */
    virtual void toggleFpsOverlay() = 0;

    /**
     * Attempts to change a display-owned effect control by relative offset.
     *
     * @param control Effect control to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int changeDisplayEffectControlBy(
        EffectControl& control, int by, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to change a display-owned effect control by choice text.
     *
     * @param control Effect control to inspect and possibly change.
     * @param to Choice text to select.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int changeDisplayEffectControlTo(
        EffectControl& control, const char* to, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to activate a display-owned effect-control choice.
     *
     * @param control Effect control to inspect and possibly change.
     * @param index Choice index to activate.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int activateDisplayEffectControl(
        EffectControl& control, int index, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to change a display-owned option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the option belongs to display.
     */
    virtual int changeDisplayOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to change a display-owned option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the option belongs to display.
     */
    virtual int changeDisplayOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) = 0;
};

/**
 * RuntimeDisplayControls implementation backed by the current display globals.
 *
 * This adapter is intentionally small: it names the display command boundary
 * while the display subsystem is still being migrated away from global options.
 */
class DefaultRuntimeDisplayControls : public RuntimeDisplayControls {
    RandomSource& randomSource;
    DisplayPresentationSettings& settings;

public:
    /**
     * Creates a display control adapter with the application-owned random
     * source used by presentation selection fallbacks.
     *
     * @param randomSource Random source owned by the application lifecycle.
     */
    DefaultRuntimeDisplayControls(RandomSource& randomSource,
        DisplayPresentationSettings& settings);

    /**
     * Changes the selected screen/presentation by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changePresentationBy(int by);

    /**
     * Changes the selected screen/presentation by name.
     *
     * @param to Choice text to select.
     */
    virtual void changePresentationTo(const char* to);

    /**
     * Changes the zoom option by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changeZoomBy(int by);

    /**
     * Changes the zoom option by name/value text.
     *
     * @param to Value text to select.
     */
    virtual void changeZoomTo(const char* to);

    /** Changes the max-FPS option by absolute integer value. */
    virtual void changeMaxFpsTo(int to);

    /** Toggles the FPS overlay. */
    virtual void toggleFpsOverlay();

    /**
     * Attempts to change a display-owned effect control by relative offset.
     *
     * @param control Effect control to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int changeDisplayEffectControlBy(
        EffectControl& control, int by, RuntimeChangeSet& changes);

    /**
     * Attempts to change a display-owned effect control by choice text.
     *
     * @param control Effect control to inspect and possibly change.
     * @param to Choice text to select.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int changeDisplayEffectControlTo(
        EffectControl& control, const char* to, RuntimeChangeSet& changes);

    /**
     * Attempts to activate a display-owned effect-control choice.
     *
     * @param control Effect control to inspect and possibly change.
     * @param index Choice index to activate.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the effect control belongs to display.
     */
    virtual int activateDisplayEffectControl(
        EffectControl& control, int index, RuntimeChangeSet& changes);

    /**
     * Attempts to change a display-owned option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the option belongs to display.
     */
    virtual int changeDisplayOptionBy(
        Option& option, int by, RuntimeChangeSet& changes);

    /**
     * Attempts to change a display-owned option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge display-side effects into.
     * @return Nonzero when the option belongs to display.
     */
    virtual int changeDisplayOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes);
};

#endif
