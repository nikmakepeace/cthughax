// Explicit registry for selectable effect controls.

#include "EffectRegistry.h"

#include "EffectControl.h"
#include "ProcessServices.h"

int EffectRegistry::indexOf(const EffectControl& control) const {
    for (unsigned int i = 0; i < controls.size(); i++)
        if (controls[i] == &control)
            return int(i);

    return -1;
}

void EffectRegistry::clear() {
    controls.clear();
}

void EffectRegistry::registerControl(EffectControl& control) {
    if (indexOf(control) >= 0)
        return;

    controls.push_back(&control);
}

int EffectRegistry::count() const {
    return int(controls.size());
}

EffectControl* EffectRegistry::controlAt(int index) const {
    if (index < 0 || index >= count())
        return 0;

    return controls[index];
}

void EffectRegistry::saveAll() {
    for (unsigned int i = 0; i < controls.size(); i++)
        controls[i]->doSave();
}

void EffectRegistry::restoreAll() {
    for (unsigned int i = 0; i < controls.size(); i++)
        controls[i]->doRestore();
}

void EffectRegistry::changeAll(RandomSource& randomSource) {
    saveAll();

    for (unsigned int i = 0; i < controls.size(); i++)
        if (controls[i]->isAutoChangeCandidate())
            controls[i]->changeRandom(randomSource, 0);
}

EffectControl* EffectRegistry::changeOne(RandomSource& randomSource) {
    int nCandidates = 0;
    for (unsigned int i = 0; i < controls.size(); i++)
        if (controls[i]->isAutoChangeCandidate())
            nCandidates++;

    if (nCandidates == 0)
        return 0;

    int n = randomSource.uniformInt(nCandidates);
    EffectControl* selected = 0;
    for (unsigned int i = 0; i < controls.size(); i++) {
        if (!controls[i]->isAutoChangeCandidate())
            continue;
        if (n == 0) {
            selected = controls[i];
            break;
        }
        n--;
    }

    if (selected == 0) {
        CTH_ERROR("internal error: no EffectControl found among %d registered autochange candidates\n",
            nCandidates);
        return 0;
    }

    if (selected->lock)
        return 0;

    saveAll();
    selected->changeRandom(randomSource, 0);
    return selected;
}
