// Lazy startup policy resolver for registered EffectControls.

#include "EffectControlPolicy.h"

#include "Configuration.h"
#include "EffectControl.h"
#include "EffectPresetCatalog.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <strings.h>
#include <vector>

namespace {

static int policyConfigured = 0;
static std::vector<EffectChoicePolicy> configuredAllowedChoices;
static std::vector<EffectPresetPolicy> configuredPresets;

static bool controlMatchesCatalogEntryKey(EffectControl& option,
    const std::string& catalogEntryKey, std::string* choiceName) {
    if (option.bufferIndex() > 0)
        return false;

    std::string catalogName = option.name();
    std::string::size_type catalogNameLength = catalogName.size();
    if (catalogEntryKey.size() <= catalogNameLength)
        return false;
    if (catalogEntryKey[catalogNameLength] != '.')
        return false;
    if (strncasecmp(catalogEntryKey.c_str(), catalogName.c_str(),
            catalogNameLength)
        != 0)
        return false;

    *choiceName = catalogEntryKey.substr(catalogNameLength + 1);
    return true;
}

static bool controlMatchesCatalogName(EffectControl& option,
    const std::string& catalogName) {
    return strcasecmp(option.name(), catalogName.c_str()) == 0;
}

static EffectChoice* findEffectChoice(EffectControl& option,
    const std::string& choiceName, int* index) {
    for (int i = 0; i < option.getNEntries(); i++) {
        if (option[i]->sameName(choiceName.c_str())) {
            if (index != NULL)
                *index = i;
            return option[i];
        }
    }

    return NULL;
}

static bool parseChoiceNumber(const std::string& text, int choiceCount,
    int* selection) {
    if (choiceCount <= 0)
        return false;

    char* end = NULL;
    long value = std::strtol(text.c_str(), &end, 0);
    if (text.empty() || end == text.c_str())
        return false;
    while (*end != '\0') {
        if (!isspace(static_cast<unsigned char>(*end)))
            return false;
        end++;
    }

    int numericValue = int(value);
    *selection = ((numericValue % choiceCount) + choiceCount) % choiceCount;
    return true;
}

static bool resolveChoiceSelection(EffectControl& option,
    const std::string& choiceText, int* selection) {
    if (findEffectChoice(option, choiceText, selection) != NULL)
        return true;

    return parseChoiceNumber(choiceText, option.getNEntries(), selection);
}

static void applyAllowedChoicePolicy(EffectControl& option) {
    for (std::vector<EffectChoicePolicy>::const_iterator it
         = configuredAllowedChoices.begin();
         it != configuredAllowedChoices.end(); ++it) {
        std::string choiceName;
        if (!controlMatchesCatalogEntryKey(option, it->catalogEntryKey,
                &choiceName))
            continue;

        EffectChoice* choice = findEffectChoice(option, choiceName, NULL);
        if (choice != NULL)
            choice->setUse(it->enabled);
    }
}

static void applyPresetPolicy(EffectControl& option) {
    for (std::vector<EffectPresetPolicy>::const_iterator it
         = configuredPresets.begin();
         it != configuredPresets.end(); ++it) {
        int selection = 0;

        if (!controlMatchesCatalogName(option, it->catalogName))
            continue;
        if (!resolveChoiceSelection(option, it->choiceText, &selection))
            continue;

        effectPresetCatalog.setValue(it->slot, option, selection);
    }
}

}

void configureEffectPolicy(const EffectPolicy& policy) {
    configuredAllowedChoices = policy.allowedChoices;
    configuredPresets = policy.presets;
    policyConfigured = 1;

    for (EffectControl* option = EffectControl::firstRegistered();
         option != NULL; option = option->nextRegistered()) {
        effectControlPolicyObserve(*option);
    }
}

void effectControlPolicyObserve(EffectControl& option) {
    if (!policyConfigured)
        return;

    applyAllowedChoicePolicy(option);
    applyPresetPolicy(option);
}
