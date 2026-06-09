// Native Scene catalog owner for wave-object line lists.

#include "SceneWaveObjectCatalog.h"

#include <string.h>

namespace {

static int waveObjectLineIsTerminator(const WObject& line) {
    for (int endpoint = 0; endpoint < 2; endpoint++) {
        for (int axis = 0; axis < 3; axis++) {
            if (line[endpoint][axis] != -1)
                return 0;
        }
    }

    return 1;
}

static WObject* copyWaveObject(const WObject* object) {
    if (object == 0)
        return 0;

    int lineCount = 0;
    while (!waveObjectLineIsTerminator(object[lineCount]))
        lineCount++;

    WObject* copy = new WObject[lineCount + 1];
    for (int i = 0; i <= lineCount; i++)
        memcpy(copy[i], object[i], sizeof(WObject));

    return copy;
}

}

SceneWaveObjectCatalog::Entry::Entry(
    const char* name_, const WObject* object_, int inUse_)
    : nameValue((name_ != 0) ? name_ : "")
    , objectValue(copyWaveObject(object_))
    , inUseValue(inUse_) { }

const char* SceneWaveObjectCatalog::Entry::name() const {
    return nameValue.c_str();
}

const WObject* SceneWaveObjectCatalog::Entry::object() const {
    return objectValue.get();
}

int SceneWaveObjectCatalog::Entry::inUse() const {
    return inUseValue;
}

SceneWaveObjectCatalog::SceneWaveObjectCatalog()
    : entries() { }

void SceneWaveObjectCatalog::clear() {
    entries.clear();
}

void SceneWaveObjectCatalog::addChoice(
    const char* name, const WObject* object, int inUse) {
    entries.push_back(std::unique_ptr<Entry>(
        new Entry(name, object, inUse)));
}

int SceneWaveObjectCatalog::entryCount() const {
    return int(entries.size());
}

const char* SceneWaveObjectCatalog::nameAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return "";

    return entries[index]->name();
}

const WObject* SceneWaveObjectCatalog::objectAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->object();
}

int SceneWaveObjectCatalog::inUseAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->inUse();
}
