/** @file
 * Runtime auto-change control port and default implementation.
 */

#ifndef CTHUGHA_RUNTIME_AUTO_CHANGE_CONTROLS_H
#define CTHUGHA_RUNTIME_AUTO_CHANGE_CONTROLS_H

#include "RuntimeCommandSink.h"

class AutoChangeControls;
class Option;

/** Controls runtime automatic scene-change settings. */
class RuntimeAutoChangeControls {
public:
    /** Destroys the auto-change controls interface. */
    virtual ~RuntimeAutoChangeControls() { }

    /** Toggles automatic scene-change lock setting. */
    virtual void toggleLock() = 0;

    /**
     * Attempts to change an automatic scene-change option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge auto-change effects into.
     * @return Nonzero when the option belongs to automatic scene-change behavior.
     */
    virtual int changeAutoChangeOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to change an automatic scene-change option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge auto-change effects into.
     * @return Nonzero when the option belongs to automatic scene-change behavior.
     */
    virtual int changeAutoChangeOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) = 0;
};

/**
 * RuntimeAutoChangeControls implementation backed by AutoChangeControls.
 */
class DefaultRuntimeAutoChangeControls : public RuntimeAutoChangeControls {
    AutoChangeControls& autoChangeControls;
    Option& quietMessageOption;

public:
    /**
     * Creates settings-backed runtime auto-change controls.
     *
     * @param autoChangeControls_ Option adapters backed by owned settings.
     * @param quietMessageOption_ Frame Generator quiet-message threshold option.
     */
    explicit DefaultRuntimeAutoChangeControls(
        AutoChangeControls& autoChangeControls_, Option& quietMessageOption_);

    /** Toggles automatic scene-change lock setting. */
    virtual void toggleLock();

    /**
     * Attempts to change an automatic scene-change option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge auto-change effects into.
     * @return Nonzero when the option belongs to automatic scene-change behavior.
     */
    virtual int changeAutoChangeOptionBy(
        Option& option, int by, RuntimeChangeSet& changes);

    /**
     * Attempts to change an automatic scene-change option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge auto-change effects into.
     * @return Nonzero when the option belongs to automatic scene-change behavior.
     */
    virtual int changeAutoChangeOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes);
};

#endif
