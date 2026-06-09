/** @file
 * Runtime command target adapters for current subsystem ownership.
 */

#include "RuntimeCommandTargets.h"

#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeDisplayControls.h"
#include "RuntimeEffectControls.h"

RoutedRuntimeOptionTarget::RoutedRuntimeOptionTarget(Option& option_,
    RuntimeDisplayControls& displayControls_,
    RuntimeAudioControls& audioControls_,
    RuntimeAutoChangeControls& autoChangeControls_)
    : option(option_)
    , displayControls(displayControls_)
    , audioControls(audioControls_)
    , autoChangeControls(autoChangeControls_) { }

RuntimeChangeSet RoutedRuntimeOptionTarget::changeBy(int by) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayOptionBy(option, by, changes))
        return changes;
    if (audioControls.changeAudioOptionBy(option, by, changes))
        return changes;
    if (autoChangeControls.changeAutoChangeOptionBy(option, by, changes))
        return changes;

    return changes;
}

RuntimeChangeSet RoutedRuntimeOptionTarget::changeTo(const char* to) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayOptionTo(option, to, changes))
        return changes;
    if (audioControls.changeAudioOptionTo(option, to, changes))
        return changes;
    if (autoChangeControls.changeAutoChangeOptionTo(option, to, changes))
        return changes;

    return changes;
}

RoutedRuntimeEffectControlTarget::RoutedRuntimeEffectControlTarget(
    EffectControl& control_, RuntimeDisplayControls& displayControls_,
    RuntimeEffectControls& effectControls_)
    : control(control_)
    , displayControls(displayControls_)
    , effectControls(effectControls_) { }

RuntimeChangeSet RoutedRuntimeEffectControlTarget::changeBy(int by) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayEffectControlBy(control, by, changes))
        return changes;

    changes.merge(effectControls.changeEffectControlBy(control, by));
    return changes;
}

RuntimeChangeSet RoutedRuntimeEffectControlTarget::changeTo(const char* to) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayEffectControlTo(control, to, changes))
        return changes;

    changes.merge(effectControls.changeEffectControlTo(control, to));
    return changes;
}

RuntimeChangeSet RoutedRuntimeEffectControlTarget::activate(int index) {
    RuntimeChangeSet changes;

    if (displayControls.activateDisplayEffectControl(control, index, changes))
        return changes;

    changes.merge(effectControls.activateEffectControl(control, index));
    return changes;
}

void RoutedRuntimeEffectControlTarget::toggleLock() {
    effectControls.toggleEffectControlLock(control);
}

void RoutedRuntimeEffectControlTarget::toggleChoiceUse(int index) {
    effectControls.toggleEffectChoiceUse(control, index);
}

RoutedRuntimeCommandTargetRouter::RoutedRuntimeCommandTargetRouter(
    RuntimeCommandSink& runtimeCommands_,
    RuntimeDisplayControls& displayControls_,
    RuntimeAudioControls& audioControls_,
    RuntimeAutoChangeControls& autoChangeControls_,
    RuntimeEffectControls& effectControls_)
    : runtimeCommands(runtimeCommands_)
    , displayControls(displayControls_)
    , audioControls(audioControls_)
    , autoChangeControls(autoChangeControls_)
    , effectControls(effectControls_) { }

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::changeEffectControlBy(
    EffectControl& control, int by) {
    RoutedRuntimeEffectControlTarget target(control, displayControls,
        effectControls);
    return runtimeCommands.apply(
        RuntimeCommand::changeEffectControlBy(target, by));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::changeEffectControlTo(
    EffectControl& control, const char* to) {
    RoutedRuntimeEffectControlTarget target(control, displayControls,
        effectControls);
    return runtimeCommands.apply(
        RuntimeCommand::changeEffectControlTo(target, to));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::activateEffectControl(
    EffectControl& control, int index) {
    RoutedRuntimeEffectControlTarget target(control, displayControls,
        effectControls);
    return runtimeCommands.apply(
        RuntimeCommand::activateEffectControl(target, index));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::toggleEffectChoiceUse(
    EffectControl& control, int index) {
    RoutedRuntimeEffectControlTarget target(control, displayControls,
        effectControls);
    return runtimeCommands.apply(
        RuntimeCommand::toggleEffectChoiceUse(target, index));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::changeOptionBy(
    Option& option, int by) {
    RoutedRuntimeOptionTarget target(option, displayControls, audioControls,
        autoChangeControls);
    return runtimeCommands.apply(RuntimeCommand::changeOptionBy(target, by));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::changeOptionTo(
    Option& option, const char* to) {
    RoutedRuntimeOptionTarget target(option, displayControls, audioControls,
        autoChangeControls);
    return runtimeCommands.apply(RuntimeCommand::changeOptionTo(target, to));
}

RuntimeChangeSet RoutedRuntimeCommandTargetRouter::toggleEffectControlLock(
    EffectControl& control) {
    RoutedRuntimeEffectControlTarget target(control, displayControls,
        effectControls);
    return runtimeCommands.apply(
        RuntimeCommand::toggleEffectControlLock(target));
}
