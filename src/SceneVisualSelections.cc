// Visual selection ports used by Scene visual catalog adapters.

#include "SceneVisualSelections.h"

#include <cctype>
#include <cstdlib>

SceneOptionSelection::~SceneOptionSelection() { }

const char* SceneOptionSelection::catalogName() const {
    return "";
}

int SceneOptionSelection::choiceCount() const {
    return 0;
}

SceneChoice* SceneOptionSelection::choiceAt(int) {
    return 0;
}

const SceneChoice* SceneOptionSelection::choiceAt(int) const {
    return 0;
}

int SceneOptionSelection::resolveValue(const char* text, int* selection) const {
    int nEntries = entryCount();
    if (selection == 0 || nEntries <= 0 || text == 0 || text[0] == '\0')
        return 0;

    char* end = 0;
    long value = std::strtol(text, &end, 0);
    if (end == text)
        return 0;
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end)))
            return 0;
        end++;
    }

    int numericValue = int(value);
    *selection = ((numericValue % nEntries) + nEntries) % nEntries;
    return 1;
}

void SceneOptionSelection::activate(int index) {
    if ((index >= 0) && (index < entryCount()))
        setValue(index);
}

int SceneOptionSelection::lockEnabled() const {
    return 0;
}

void SceneOptionSelection::toggleLock() { }

void SceneOptionSelection::toggleChoiceUse(int) { }

SceneVisualSelections::~SceneVisualSelections() { }
