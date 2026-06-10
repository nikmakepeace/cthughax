/** @file
 * Display catalog view used by the generic control IPC layer.
 */

#include "ControlDisplayCatalogs.h"

#include "Screen.h"

int BuiltInControlDisplayCatalogs::screenChoiceCount() const {
    return nScreenCatalogEntries;
}

const char* BuiltInControlDisplayCatalogs::screenChoiceNameAt(int index) const {
    ScreenEntry* entry = screenByIndex(index);
    return entry != 0 ? entry->Name() : "";
}

const char* BuiltInControlDisplayCatalogs::screenChoiceLabelAt(int index) const {
    ScreenEntry* entry = screenByIndex(index);
    return entry != 0 ? entry->Desc() : "";
}

int BuiltInControlDisplayCatalogs::screenChoiceInUseAt(int index) const {
    ScreenEntry* entry = screenByIndex(index);
    return entry != 0 ? entry->inUse() : 0;
}
