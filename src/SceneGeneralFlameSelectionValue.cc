// Native encoded general-flame Scene selection.

#include "SceneGeneralFlameSelectionValue.h"

#include "ProcessServices.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace {

static const int generalFlameStates = 9 * 9 * 9 * 9 * 9;

static int modInt(int value, int modulo) {
    int result = value % modulo;
    return result < 0 ? result + modulo : result;
}

}

SceneGeneralFlameSelectionValue::SceneGeneralFlameSelectionValue(
    const char* catalogName_, SceneChoiceLock* lock_, int selectedValue_,
    LogSink* log_)
    : catalogNameValue((catalogName_ != 0) ? catalogName_ : "")
    , lockValue(lock_)
    , selectedValue(modInt(selectedValue_, generalFlameStates))
    , logValue(log_) {
    selectionTextValue[0] = '\0';
}

const char* SceneGeneralFlameSelectionValue::applyLockPrefix(
    const char* text) {
    static const char* lockOn[] = { "l:", "lock:", "locked:" };
    static const char* lockOff[] = { "no-l:", "no-lock:", "no-locked:",
        "nol:", "nolock:", "nolocked:", "non-l:", "non-lock:",
        "non-locked:", "nonl:", "nonlock:", "nonlocked:" };

    if (text == 0)
        return "";

    for (unsigned int i = 0; i < sizeof(lockOn) / sizeof(lockOn[0]); i++) {
        if (strncasecmp(text, lockOn[i], std::strlen(lockOn[i])) == 0) {
            if (lockValue.get() != 0)
                lockValue->change("on");
            return text + std::strlen(lockOn[i]);
        }
    }

    for (unsigned int i = 0; i < sizeof(lockOff) / sizeof(lockOff[0]); i++) {
        if (strncasecmp(text, lockOff[i], std::strlen(lockOff[i])) == 0) {
            if (lockValue.get() != 0)
                lockValue->change("off");
            return text + std::strlen(lockOff[i]);
        }
    }

    return text;
}

const char* SceneGeneralFlameSelectionValue::catalogName() const {
    return catalogNameValue.c_str();
}

const char* SceneGeneralFlameSelectionValue::currentName() const {
    return selectionText();
}

int SceneGeneralFlameSelectionValue::currentValue() const {
    return encodedValue();
}

int SceneGeneralFlameSelectionValue::entryCount() const {
    return generalFlameStates;
}

void SceneGeneralFlameSelectionValue::change(int by) {
    setValue(encodedValue() + by);
}

void SceneGeneralFlameSelectionValue::change(
    const char* to, RandomSource& randomSource) {
    char* pos;

    if ((to == 0) || (to[0] == '\0'))
        return;

    to = applyLockPrefix(to);

    int newValue = std::strtol(to, &pos, 0);
    if (pos == to) {
        if (logValue != 0)
            logValue->warn("Unknown entry `%s' for option `%s'\n", to,
                catalogName());
        changeRandom(randomSource);
        return;
    }

    setValue(newValue);
}

int SceneGeneralFlameSelectionValue::changeRandom(
    RandomSource& randomSource) {
    if (lockValue.get() != 0 && lockValue->enabled())
        return 0;

    int previousValue = encodedValue();
    setValue(randomSource.uniformInt(generalFlameStates));
    return encodedValue() != previousValue;
}

void SceneGeneralFlameSelectionValue::activate(int index) {
    if ((index >= 0) && (index < entryCount()))
        setValue(index);
}

int SceneGeneralFlameSelectionValue::lockEnabled() const {
    return lockValue.get() != 0 && lockValue->enabled();
}

void SceneGeneralFlameSelectionValue::toggleLock() {
    if (lockValue.get() != 0)
        lockValue->change(lockValue->enabled() ? "off" : "on");
}

void SceneGeneralFlameSelectionValue::setValue(int index) {
    selectedValue = modInt(index, generalFlameStates);
}

int SceneGeneralFlameSelectionValue::encodedValue() const {
    return selectedValue;
}

const char* SceneGeneralFlameSelectionValue::selectionText() const {
    if (lockValue.get() != 0 && lockValue->enabled())
        std::snprintf(selectionTextValue, sizeof(selectionTextValue),
            "locked:%d", encodedValue());
    else
        std::snprintf(selectionTextValue, sizeof(selectionTextValue), "%d",
            encodedValue());

    return selectionTextValue;
}
