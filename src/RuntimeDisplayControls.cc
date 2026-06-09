/** @file
 * Runtime display/presentation control adapter.
 */

#include "RuntimeDisplayControls.h"

#include "DisplayPresentationOptions.h"
#include "ProcessServices.h"
#include "Screen.h"

DefaultRuntimeDisplayControls::DefaultRuntimeDisplayControls(RandomSource& randomSource_)
    : randomSource(randomSource_) { }

void DefaultRuntimeDisplayControls::changePresentationBy(int by) {
    screen.change(by, 0);
}

void DefaultRuntimeDisplayControls::changePresentationTo(const char* to) {
    screen.change(to, randomSource, 0);
}

void DefaultRuntimeDisplayControls::changeZoomBy(int by) {
    zoom.change(by);
}

void DefaultRuntimeDisplayControls::changeZoomTo(const char* to) {
    zoom.change(to);
}

void DefaultRuntimeDisplayControls::toggleFpsOverlay() {
    showFPS.change(+1);
}

int DefaultRuntimeDisplayControls::changeDisplayEffectControlBy(
    EffectControl& control, int by, RuntimeChangeSet& changes) {
    if (&control != &screen)
        return 0;

    changePresentationBy(by);
    changes.displayChanged = 1;
    return 1;
}

int DefaultRuntimeDisplayControls::changeDisplayEffectControlTo(
    EffectControl& control, const char* to, RuntimeChangeSet& changes) {
    if (&control != &screen)
        return 0;

    changePresentationTo(to);
    changes.displayChanged = 1;
    return 1;
}

int DefaultRuntimeDisplayControls::activateDisplayEffectControl(
    EffectControl& control, int index, RuntimeChangeSet& changes) {
    if (&control != &screen)
        return 0;

    if ((index >= 0) && (index < control.getNEntries())) {
        control[index]->setUse(1);
        control.setValue(index);
        control.change(0, 0);
        changes.displayChanged = 1;
    }

    return 1;
}

int DefaultRuntimeDisplayControls::changeDisplayOptionBy(
    Option& option, int by, RuntimeChangeSet& changes) {
    if ((&option != &zoom) && (&option != &maxFramesPerSecond)
        && (&option != &showFPS))
        return 0;

    option.change(by);
    changes.displayChanged = (&option == &zoom);
    changes.fpsChanged = (&option == &showFPS);
    return 1;
}

int DefaultRuntimeDisplayControls::changeDisplayOptionTo(
    Option& option, const char* to, RuntimeChangeSet& changes) {
    if ((&option != &zoom) && (&option != &maxFramesPerSecond)
        && (&option != &showFPS))
        return 0;

    option.change(to);
    changes.displayChanged = (&option == &zoom);
    changes.fpsChanged = (&option == &showFPS);
    return 1;
}
