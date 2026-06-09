/** @file
 * Runtime EffectControl state adapter.
 */

#include "RuntimeEffectControls.h"

#include "EffectControl.h"
#include "ProcessServices.h"

DefaultRuntimeEffectControls::DefaultRuntimeEffectControls(RandomSource& randomSource_)
    : randomSource(randomSource_) { }

RuntimeChangeSet DefaultRuntimeEffectControls::changeEffectControlBy(
    EffectControl& control, int by) {
    RuntimeChangeSet changes;

    control.change(by, 0);

    return changes;
}

RuntimeChangeSet DefaultRuntimeEffectControls::changeEffectControlTo(
    EffectControl& control, const char* to) {
    RuntimeChangeSet changes;

    control.change(to, randomSource, 0);

    return changes;
}

RuntimeChangeSet DefaultRuntimeEffectControls::activateEffectControl(
    EffectControl& control, int index) {
    RuntimeChangeSet changes;

    if ((index >= 0) && (index < control.getNEntries())) {
        control[index]->setUse(1);
        control.setValue(index);
        control.change(0, 0);
    }

    return changes;
}

void DefaultRuntimeEffectControls::toggleEffectControlLock(
    EffectControl& control) {
    control.lock.change(+1);
}

void DefaultRuntimeEffectControls::toggleEffectChoiceUse(
    EffectControl& control, int index) {
    if ((index < 0) || (index >= control.getNEntries()))
        return;

    control[index]->setUse(!control[index]->inUse());
}
