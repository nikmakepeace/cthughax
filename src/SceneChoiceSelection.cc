// Catalog-backed Scene visual selection state.

#include "SceneChoiceSelection.h"

#include "ProcessServices.h"
#include "cthugha.h"

#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace {

static int modInt(int value, int modulo) {
    int result = value % modulo;
    return result < 0 ? result + modulo : result;
}

}

SceneChoice::~SceneChoice() { }

SceneChoiceLock::~SceneChoiceLock() { }

SceneChoiceLockValue::SceneChoiceLockValue(int enabled_)
    : enabledValue(enabled_ != 0) { }

int SceneChoiceLockValue::enabled() const {
    return enabledValue;
}

void SceneChoiceLockValue::change(const char* to) {
    if (to == 0)
        return;

    if (!strncasecmp("yes", to, 3) || !strncasecmp("on", to, 2)
        || !strncasecmp("1", to, 1)) {
        enabledValue = 1;
    } else if (!strncasecmp("no", to, 2) || !strncasecmp("off", to, 3)
        || !strncasecmp("0", to, 1)) {
        enabledValue = 0;
    } else {
        CTH_ERROR("Illegal yes/no-value `%s'.\n", to);
    }
}

SceneChoiceCatalog::~SceneChoiceCatalog() { }

SceneChoiceSelection::SceneChoiceSelection(
    SceneChoiceCatalog* catalog_, int selectedValue_)
    : catalog(catalog_)
    , choices()
    , catalogEntryCount(-1)
    , selectedValue(selectedValue_) { }

SceneChoiceLock& SceneChoiceSelection::selectionLock() {
    return catalog->lock();
}

const SceneChoiceLock& SceneChoiceSelection::selectionLock() const {
    return catalog->lock();
}

const char* SceneChoiceSelection::optionName() const {
    return catalog->optionName();
}

const char* SceneChoiceSelection::catalogName() const {
    return optionName();
}

const char* SceneChoiceSelection::applySelectionLockPrefix(const char* to) {
    static const char* lockOn[] = { "l:", "lock:", "locked:" };
    static const char* lockOff[] = { "no-l:", "no-lock:", "no-locked:", "nol:", "nolock:",
        "nolocked:", "non-l:", "non-lock:", "non-locked:", "nonl:", "nonlock:",
        "nonlocked:" };

    if (to == 0)
        return "";

    for (unsigned int i = 0; i < sizeof(lockOn) / sizeof(lockOn[0]); i++) {
        if (strncasecmp(to, lockOn[i], std::strlen(lockOn[i])) == 0) {
            selectionLock().change("on");
            return to + std::strlen(lockOn[i]);
        }
    }

    for (unsigned int i = 0; i < sizeof(lockOff) / sizeof(lockOff[0]); i++) {
        if (strncasecmp(to, lockOff[i], std::strlen(lockOff[i])) == 0) {
            selectionLock().change("off");
            return to + std::strlen(lockOff[i]);
        }
    }

    return to;
}

void SceneChoiceSelection::refreshCatalog() const {
    int nEntries = catalog->entryCount();
    if (catalogEntryCount == nEntries)
        return;

    choices.clear();
    for (int i = 0; i < nEntries; i++)
        choices.push_back(catalog->choiceAt(i));
    catalogEntryCount = nEntries;
}

SceneChoice* SceneChoiceSelection::choiceAt(int index) {
    refreshCatalog();
    if ((index < 0) || (index >= int(choices.size())))
        return 0;
    return choices[index];
}

const SceneChoice* SceneChoiceSelection::choiceAt(int index) const {
    refreshCatalog();
    if ((index < 0) || (index >= int(choices.size())))
        return 0;
    return choices[index];
}

SceneChoice* SceneChoiceSelection::currentChoice() {
    return choiceAt(selectedValue);
}

const SceneChoice* SceneChoiceSelection::currentChoice() const {
    return choiceAt(selectedValue);
}

void SceneChoiceSelection::setSelectedValue(int index) {
    selectedValue = index;
}

void SceneChoiceSelection::selectionChanged() { }

const char* SceneChoiceSelection::currentName() const {
    const SceneChoice* choice = currentChoice();
    return (choice != 0) ? choice->name() : "unknown";
}

int SceneChoiceSelection::currentValue() const {
    return selectedValue;
}

int SceneChoiceSelection::entryCount() const {
    refreshCatalog();
    return int(choices.size());
}

int SceneChoiceSelection::choiceCount() const {
    return SceneChoiceSelection::entryCount();
}

int SceneChoiceSelection::resolveValue(const char* text, int* selection) const {
    int nChoices = choiceCount();

    if (text == 0)
        return 0;

    for (int i = 0; i < nChoices; i++) {
        const SceneChoice* choice = choiceAt(i);
        if (choice != 0 && choice->sameName(text)) {
            if (selection != 0)
                *selection = i;
            return 1;
        }
    }

    return SceneOptionSelection::resolveValue(text, selection);
}

void SceneChoiceSelection::change(int by) {
    int nEntries = entryCount();
    if (nEntries == 0)
        return;

    int noChoicesInUse = 1;
    for (int i = 0; i < nEntries; i++) {
        SceneChoice* choice = choiceAt(i);
        if (choice != 0 && choice->inUse()) {
            noChoicesInUse = 0;
            break;
        }
    }

    if (noChoicesInUse) {
        selectedValue = 0;
    } else {
        selectedValue = modInt(selectedValue + by, nEntries);
        if (by < 0) {
            while (choiceAt(selectedValue) != 0
                && !choiceAt(selectedValue)->inUse())
                selectedValue = modInt(selectedValue - 1, nEntries);
        } else {
            while (choiceAt(selectedValue) != 0
                && !choiceAt(selectedValue)->inUse())
                selectedValue = modInt(selectedValue + 1, nEntries);
        }
    }

    selectionChanged();
}

void SceneChoiceSelection::change(
    const char* to, RandomSource& randomSource) {
    char* pos;

    int nEntries = entryCount();
    if (nEntries == 0)
        return;

    if ((to == 0) || (to[0] == '\0')) {
        selectedValue = randomSource.uniformInt(nEntries);
        change(0);
        return;
    }

    to = applySelectionLockPrefix(to);

    for (int i = 0; i < nEntries; i++) {
        SceneChoice* choice = choiceAt(i);
        if (choice != 0 && choice->sameName(to)) {
            choice->setUse(1);
            selectedValue = i;
            selectionChanged();
            return;
        }
    }

    int parsedValue = std::strtol(to, &pos, 0);
    if (pos == to) {
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to, optionName());
        selectedValue = randomSource.uniformInt(nEntries);
    } else {
        selectedValue = modInt(parsedValue, nEntries);
    }

    SceneChoice* choice = currentChoice();
    if (choice != 0)
        choice->setUse(1);
    selectionChanged();
}

int SceneChoiceSelection::changeRandom(RandomSource& randomSource) {
    if (selectionLock().enabled())
        return 0;

    int nEntries = entryCount();
    if (nEntries == 0)
        return 0;

    int previousValue = selectedValue;
    selectedValue = randomSource.uniformInt(nEntries);
    change(0);
    return selectedValue != previousValue;
}

void SceneChoiceSelection::activate(int index) {
    if ((index < 0) || (index >= entryCount()))
        return;

    SceneChoice* choice = choiceAt(index);
    if (choice != 0)
        choice->setUse(1);
    selectedValue = index;
    change(0);
}

int SceneChoiceSelection::lockEnabled() const {
    return selectionLock().enabled();
}

void SceneChoiceSelection::toggleLock() {
    selectionLock().change(selectionLock().enabled() ? "off" : "on");
}

void SceneChoiceSelection::toggleChoiceUse(int index) {
    SceneChoice* choice = choiceAt(index);
    if (choice != 0)
        choice->setUse(!choice->inUse());
}

void SceneChoiceSelection::setValue(int index) {
    selectedValue = index;
    selectionChanged();
}
