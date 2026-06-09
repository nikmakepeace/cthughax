/** @file
 * Runtime command target ports for option and effect-control changes.
 */

#ifndef CTHUGHA_RUNTIME_COMMAND_TARGETS_H
#define CTHUGHA_RUNTIME_COMMAND_TARGETS_H

#include "RuntimeCommandSink.h"

class EffectControl;
class Option;
class RuntimeAudioControls;
class RuntimeAutoChangeControls;
class RuntimeCommandSink;
class RuntimeDisplayControls;
class RuntimeEffectControls;

/**
 * Target for generic runtime option commands.
 */
class RuntimeOptionTarget {
public:
    /** Destroys the option target interface. */
    virtual ~RuntimeOptionTarget() { }

    /**
     * Applies a relative option change.
     *
     * @param by Relative offset to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeBy(int by) = 0;

    /**
     * Applies an absolute option change.
     *
     * @param to Choice/value text to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeTo(const char* to) = 0;
};

/**
 * Target for generic runtime effect-control commands.
 */
class RuntimeEffectControlTarget {
public:
    /** Destroys the effect-control target interface. */
    virtual ~RuntimeEffectControlTarget() { }

    /**
     * Applies a relative effect-control change.
     *
     * @param by Relative offset to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeBy(int by) = 0;

    /**
     * Applies an absolute effect-control change.
     *
     * @param to Choice/value text to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeTo(const char* to) = 0;

    /**
     * Activates a choice by index.
     *
     * @param index Choice index to activate.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet activate(int index) = 0;

    /** Toggles the effect-control lock flag. */
    virtual void toggleLock() = 0;

    /**
     * Toggles use for a contained choice.
     *
     * @param index Choice index whose use flag should toggle.
     */
    virtual void toggleChoiceUse(int index) = 0;
};

/**
 * Runtime option target that routes a legacy Option through subsystem ports.
 */
class RoutedRuntimeOptionTarget : public RuntimeOptionTarget {
    Option& option;
    RuntimeDisplayControls& displayControls;
    RuntimeAudioControls& audioControls;
    RuntimeAutoChangeControls& autoChangeControls;

public:
    /**
     * Creates a routed option target.
     *
     * @param option_ Legacy option being routed.
     * @param displayControls_ Display control port.
     * @param audioControls_ Audio control port.
     * @param autoChangeControls_ Auto-change control port.
     */
    RoutedRuntimeOptionTarget(Option& option_,
        RuntimeDisplayControls& displayControls_,
        RuntimeAudioControls& audioControls_,
        RuntimeAutoChangeControls& autoChangeControls_);

    /** Applies a relative option change through the owning subsystem. */
    virtual RuntimeChangeSet changeBy(int by);

    /** Applies an absolute option change through the owning subsystem. */
    virtual RuntimeChangeSet changeTo(const char* to);
};

/**
 * Runtime effect target that routes a legacy EffectControl through owner ports.
 */
class RoutedRuntimeEffectControlTarget : public RuntimeEffectControlTarget {
    EffectControl& control;
    RuntimeDisplayControls& displayControls;
    RuntimeEffectControls& effectControls;

public:
    /**
     * Creates a routed effect-control target.
     *
     * @param control_ Legacy effect control being routed.
     * @param displayControls_ Display control port.
     * @param effectControls_ Effect control port.
     */
    RoutedRuntimeEffectControlTarget(EffectControl& control_,
        RuntimeDisplayControls& displayControls_,
        RuntimeEffectControls& effectControls_);

    /** Applies a relative effect-control change through the owning subsystem. */
    virtual RuntimeChangeSet changeBy(int by);

    /** Applies an absolute effect-control change through the owning subsystem. */
    virtual RuntimeChangeSet changeTo(const char* to);

    /** Activates a choice through the owning subsystem. */
    virtual RuntimeChangeSet activate(int index);

    /** Toggles the effect-control lock flag. */
    virtual void toggleLock();

    /** Toggles use for a contained choice. */
    virtual void toggleChoiceUse(int index);
};

/**
 * Applies UI-originated option/effect changes through explicit command targets.
 */
class RuntimeCommandTargetRouter {
public:
    /** Destroys the target router interface. */
    virtual ~RuntimeCommandTargetRouter() { }

    /**
     * Applies a relative effect-control change.
     *
     * @param control Effect control to change.
     * @param by Relative offset to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl& control, int by) = 0;

    /**
     * Applies an absolute effect-control change.
     *
     * @param control Effect control to change.
     * @param to Choice/value text to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl& control, const char* to) = 0;

    /**
     * Activates an effect-control choice.
     *
     * @param control Effect control to activate.
     * @param index Choice index to activate.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet activateEffectControl(
        EffectControl& control, int index) = 0;

    /**
     * Toggles use for one effect-control choice.
     *
     * @param control Effect control containing the choice.
     * @param index Choice index whose use flag should toggle.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet toggleEffectChoiceUse(
        EffectControl& control, int index) = 0;

    /**
     * Applies a relative option change.
     *
     * @param option Option to change.
     * @param by Relative offset to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeOptionBy(Option& option, int by) = 0;

    /**
     * Applies an absolute option change.
     *
     * @param option Option to change.
     * @param to Choice/value text to apply.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet changeOptionTo(Option& option, const char* to) = 0;

    /**
     * Toggles the lock flag for an effect control.
     *
     * @param control Effect control whose lock should toggle.
     * @return Change flags produced by the command.
     */
    virtual RuntimeChangeSet toggleEffectControlLock(
        EffectControl& control) = 0;
};

/**
 * Runtime command target router for the current subsystem control ports.
 */
class RoutedRuntimeCommandTargetRouter : public RuntimeCommandTargetRouter {
    RuntimeCommandSink& runtimeCommands;
    RuntimeDisplayControls& displayControls;
    RuntimeAudioControls& audioControls;
    RuntimeAutoChangeControls& autoChangeControls;
    RuntimeEffectControls& effectControls;

public:
    /**
     * Creates a routed target router.
     *
     * @param runtimeCommands_ Command sink that executes target-backed commands.
     * @param displayControls_ Display control port.
     * @param audioControls_ Audio control port.
     * @param autoChangeControls_ Auto-change control port.
     * @param effectControls_ Effect control port.
     */
    RoutedRuntimeCommandTargetRouter(RuntimeCommandSink& runtimeCommands_,
        RuntimeDisplayControls& displayControls_,
        RuntimeAudioControls& audioControls_,
        RuntimeAutoChangeControls& autoChangeControls_,
        RuntimeEffectControls& effectControls_);

    /** Applies a relative effect-control change. */
    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl& control, int by);

    /** Applies an absolute effect-control change. */
    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl& control, const char* to);

    /** Activates an effect-control choice. */
    virtual RuntimeChangeSet activateEffectControl(
        EffectControl& control, int index);

    /** Toggles use for one effect-control choice. */
    virtual RuntimeChangeSet toggleEffectChoiceUse(
        EffectControl& control, int index);

    /** Applies a relative option change. */
    virtual RuntimeChangeSet changeOptionBy(Option& option, int by);

    /** Applies an absolute option change. */
    virtual RuntimeChangeSet changeOptionTo(Option& option, const char* to);

    /** Toggles the lock flag for an effect control. */
    virtual RuntimeChangeSet toggleEffectControlLock(EffectControl& control);
};

#endif
