// Ini-file adapter for registered CoreOptions.

#include "CoreOptionIni.h"

#include "cthugha.h"
#include "CoreOption.h"
#include "options.h"

#include <string>

static CoreOption* firstCoreOption() {
    return CoreOption::firstRegistered();
}

static void getCoreOptionIniUsage(CoreOption& option) {
    for (int i = 0; i < option.getNEntries(); i++) {
        std::string key = std::string(option.name()) + "." + option[i]->Name();
        int use = option[i]->inUse();
        if (!getini_yesno(key.c_str(), &use))
            option[i]->setUse(use);
    }
}

static void putCoreOptionIniUsage(CoreOption& option) {
    for (int i = 0; i < option.getNEntries(); i++) {
        std::string key = std::string(option.name()) + "." + option[i]->Name();
        putini(key.c_str(), option[i]->useText());
    }
}

void coreOptionGetIniInitials() {
    for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
        getini(*option);
    }
}

void coreOptionPutIniInitials() {
    for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
        putini(*option);
    }
}

void coreOptionGetIniUsages() {
    for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
        if (option->bufferIndex() <= 0)
            getCoreOptionIniUsage(*option);
    }
}

void coreOptionPutIniUsages() {
    for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
        if (option->bufferIndex() <= 0)
            putCoreOptionIniUsage(*option);
    }
}

void coreOptionGetHotIni() {
    for (int i = 0; i < CoreOption::hotSlotCount(); i++) {
        for (CoreOption* option = firstCoreOption(); option != NULL;
             option = option->nextRegistered()) {
            std::string key = std::string("hot.") + std::to_string(i) + "." + option->name();
            char val[512];
            if (!getini(key.c_str(), val)) {
                option->setHotValue(i, option->optNr(val));
            }
        }
    }
}

void coreOptionPutHotIni() {
    for (int i = 0; i < CoreOption::hotSlotCount(); i++) {
        for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
            std::string key = std::string("hot.") + std::to_string(i) + "." + option->name();
            putini(key.c_str(), option->text(option->hotValue(i)));
        }
    }
}

int coreOptionIsIniEntry(const char* entry) {
    int len;

    if (strchr(entry, '?') != NULL)
        return 1;

    if (strncasecmp(entry, "hot.", 4) == 0)
        return 1;

    for (CoreOption* option = firstCoreOption(); option != NULL;
         option = option->nextRegistered()) {
        if (strcasecmp(entry, option->name()) == 0)
            return 1;

        len = strlen(option->name());
        if ((strncasecmp(entry, option->name(), len) == 0) && (entry[len] == '.'))
            return 1;

        for (int i = 0; i < CoreOption::hotSlotCount(); i++) {
            std::string key = std::string("hot.") + std::to_string(i) + "." + option->name();
            if (strcasecmp(entry, key.c_str()) == 0)
                return 1;
        }

        for (int i = 0; i < option->getNEntries(); i++) {
            std::string key = std::string(option->name()) + "." + (*option)[i]->Name();
            if (strcasecmp(entry, key.c_str()) == 0)
                return 1;
        }
    }

    return 0;
}
