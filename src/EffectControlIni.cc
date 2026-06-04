// Ini-file adapter for registered EffectControls.

#include "EffectControlIni.h"

#include "cthugha.h"
#include "EffectControl.h"
#include "IniFiles.h"
#include "EffectPresetCatalog.h"

#include <string>

static EffectControl* firstEffectControl() {
    return EffectControl::firstRegistered();
}

static void putEffectControlIniUsage(EffectControl& option) {
    for (int i = 0; i < option.getNEntries(); i++) {
        std::string key = std::string(option.name()) + "." + option[i]->Name();
        putini(key.c_str(), option[i]->useText());
    }
}

void effectControlPutIniInitials() {
    for (EffectControl* option = firstEffectControl(); option != NULL;
         option = option->nextRegistered()) {
        putini(*option);
    }
}

void effectControlPutIniUsages() {
    for (EffectControl* option = firstEffectControl(); option != NULL;
         option = option->nextRegistered()) {
        if (option->bufferIndex() <= 0)
            putEffectControlIniUsage(*option);
    }
}

void effectControlPutPresetIni() {
    for (int i = 0; i < effectPresetCatalog.slotCount(); i++) {
        for (EffectControl* option = firstEffectControl(); option != NULL;
             option = option->nextRegistered()) {
            std::string key = std::string("preset.") + std::to_string(i) + "." + option->name();
            putini(key.c_str(), option->text(effectPresetCatalog.value(i, *option)));
        }
    }
}
