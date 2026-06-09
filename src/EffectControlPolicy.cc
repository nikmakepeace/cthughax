// Lazy startup policy resolver for registered EffectControls.

#include "EffectControlPolicy.h"

#include "EffectControl.h"
#include "EffectRegistry.h"
#include "EffectPresetCatalog.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <strings.h>
#include <vector>

namespace {

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

static void applyAllowedChoicePolicy(EffectControl& option,
    const std::vector<EffectChoicePolicy>& allowedChoices) {
    for (std::vector<EffectChoicePolicy>::const_iterator it
         = allowedChoices.begin();
         it != allowedChoices.end(); ++it) {
        std::string choiceName;
        if (!controlMatchesCatalogEntryKey(option, it->catalogEntryKey,
                &choiceName))
            continue;

        EffectChoice* choice = findEffectChoice(option, choiceName, NULL);
        if (choice != NULL)
            choice->setUse(it->enabled);
    }
}

static void applyPresetPolicy(EffectControl& option,
    const std::vector<EffectPresetPolicy>& presetPolicies,
    EffectPresetCatalog& presets) {
    for (std::vector<EffectPresetPolicy>::const_iterator it
         = presetPolicies.begin();
         it != presetPolicies.end(); ++it) {
        int selection = 0;

        if (!controlMatchesCatalogName(option, it->catalogName))
            continue;
        if (!resolveChoiceSelection(option, it->choiceText, &selection))
            continue;

        presets.setValue(it->slot, option, selection);
    }
}

}

EffectPolicyApplier::EffectPolicyApplier(EffectRegistry& registry_,
    EffectPresetCatalog& presets_)
    : registry(registry_)
    , presets(presets_)
    , policyConfigured(0)
    , allowedChoices()
    , presetPolicies() { }

EffectPolicyApplier::~EffectPolicyApplier() { }

void EffectPolicyApplier::configure(const EffectPolicy& policy) {
    allowedChoices = policy.allowedChoices;
    presetPolicies = policy.presets;
    policyConfigured = 1;

    applyAll();
}

void EffectPolicyApplier::observe(EffectControl& option) {
    if (!policyConfigured)
        return;

    applyAllowedChoicePolicy(option, allowedChoices);
    applyPresetPolicy(option, presetPolicies, presets);
}

void EffectPolicyApplier::applyAll() {
    if (!policyConfigured)
        return;

    for (int i = 0; i < registry.count(); i++) {
        EffectControl* option = registry.controlAt(i);
        if (option != 0)
            observe(*option);
    }
}
