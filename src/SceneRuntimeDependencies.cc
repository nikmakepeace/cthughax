// Scene runtime wiring ports and adapters.

#include "SceneRuntimeDependencies.h"

#include "ProcessServices.h"
#include "SceneChoiceSelection.h"
#include "SceneVisualSelections.h"
#include "cthugha.h"

#include <strings.h>
#include <utility>

namespace {

static bool selectionMatchesCatalogEntryKey(
    const SceneOptionSelection& selection,
    const std::string& catalogEntryKey, std::string* choiceName) {
    std::string catalogName = selection.catalogName();
    std::string::size_type catalogNameLength = catalogName.size();

    if (catalogName.empty())
        return false;
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

static bool selectionMatchesCatalogName(
    const SceneOptionSelection& selection, const std::string& catalogName) {
    return strcasecmp(selection.catalogName(), catalogName.c_str()) == 0;
}

static SceneChoice* findSelectionChoice(SceneOptionSelection& selection,
    const std::string& choiceName) {
    for (int i = 0; i < selection.choiceCount(); i++) {
        SceneChoice* choice = selection.choiceAt(i);
        if (choice != 0 && choice->sameName(choiceName.c_str()))
            return choice;
    }

    return 0;
}

static void applyAllowedChoicePolicy(SceneOptionSelection& selection,
    const std::vector<EffectChoicePolicy>& allowedChoices) {
    for (std::vector<EffectChoicePolicy>::const_iterator it
         = allowedChoices.begin();
         it != allowedChoices.end(); ++it) {
        std::string choiceName;
        if (!selectionMatchesCatalogEntryKey(
                selection, it->catalogEntryKey, &choiceName))
            continue;

        SceneChoice* choice = findSelectionChoice(selection, choiceName);
        if (choice != 0)
            choice->setUse(it->enabled);
    }
}

static void applyPresetPolicy(SceneOptionSelection& selection,
    const std::vector<EffectPresetPolicy>& presetPolicies,
    SceneSelectionPresetCatalog& presets) {
    for (std::vector<EffectPresetPolicy>::const_iterator it
         = presetPolicies.begin();
         it != presetPolicies.end(); ++it) {
        int selectionValue = 0;

        if (!selectionMatchesCatalogName(selection, it->catalogName))
            continue;
        if (!selection.resolveValue(it->choiceText.c_str(), &selectionValue))
            continue;

        presets.setValue(it->slot, selection, selectionValue);
    }
}

}

SceneVisualCatalogFactoryResult::SceneVisualCatalogFactoryResult()
    : visualCatalogs()
    , selections(0) { }

SceneVisualCatalogFactoryResult::SceneVisualCatalogFactoryResult(
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs_,
    SceneVisualSelections& selections_)
    : visualCatalogs(std::move(visualCatalogs_))
    , selections(&selections_) { }

SceneVisualCatalogFactory::~SceneVisualCatalogFactory() { }

SceneSelectionRegistry::Entry::Entry(SceneOptionSelection& selection_)
    : selection(&selection_)
    , history() { }

SceneSelectionRegistry::SceneSelectionRegistry()
    : entries() { }

int SceneSelectionRegistry::indexOf(
    const SceneOptionSelection& selection) const {
    for (unsigned int i = 0; i < entries.size(); i++)
        if (entries[i].selection == &selection)
            return int(i);

    return -1;
}

void SceneSelectionRegistry::registerSelection(
    SceneOptionSelection& selection) {
    if (indexOf(selection) >= 0)
        return;

    entries.push_back(Entry(selection));
}

void SceneSelectionRegistry::registerSelections(
    SceneVisualSelections& selections) {
    registerSelection(selections.flame());
    registerSelection(selections.generalFlame());
    registerSelection(selections.wave());
    registerSelection(selections.waveScale());
    registerSelection(selections.table());
    registerSelection(selections.object());
    registerSelection(selections.translation());
    registerSelection(selections.palette());
    registerSelection(selections.border());
    registerSelection(selections.flashlight());
    registerSelection(selections.images());
}

int SceneSelectionRegistry::count() const {
    return int(entries.size());
}

SceneOptionSelection* SceneSelectionRegistry::selectionAt(int index) const {
    if (index < 0 || index >= count())
        return 0;

    return entries[index].selection;
}

void SceneSelectionRegistry::saveEntry(Entry& entry) {
    static const unsigned int maxHistory = 128;

    if (entry.history.size() >= maxHistory)
        entry.history.erase(entry.history.begin());

    entry.history.push_back(entry.selection->currentValue());
}

void SceneSelectionRegistry::restoreEntry(Entry& entry) {
    if (entry.history.empty())
        return;

    int value = entry.history.back();
    entry.history.pop_back();
    entry.selection->setValue(value);
}

void SceneSelectionRegistry::saveAll() {
    for (unsigned int i = 0; i < entries.size(); i++)
        saveEntry(entries[i]);
}

void SceneSelectionRegistry::restoreAll() {
    for (unsigned int i = 0; i < entries.size(); i++)
        restoreEntry(entries[i]);
}

void SceneSelectionRegistry::changeAll(RandomSource& randomSource) {
    saveAll();

    for (unsigned int i = 0; i < entries.size(); i++)
        entries[i].selection->changeRandom(randomSource);
}

void SceneSelectionRegistry::changeOne(RandomSource& randomSource) {
    int nCandidates = count();
    if (nCandidates == 0)
        return;

    int index = randomSource.uniformInt(nCandidates);
    SceneOptionSelection* selection = selectionAt(index);
    if (selection == 0) {
        CTH_ERROR("internal error: no Scene selection found among %d candidates\n",
            nCandidates);
        return;
    }

    saveAll();
    selection->changeRandom(randomSource);
}

SceneSelectionPresetCatalog::Value::Value(
    SceneOptionSelection& selection_, int selectionValue_)
    : selection(&selection_)
    , selectionValue(selectionValue_) { }

int SceneSelectionPresetCatalog::Slot::indexOf(
    const SceneOptionSelection& selection) const {
    for (unsigned int i = 0; i < values.size(); i++)
        if (values[i].selection == &selection)
            return int(i);

    return -1;
}

void SceneSelectionPresetCatalog::Slot::setValue(
    SceneOptionSelection& selection, int selectionValue) {
    int index = indexOf(selection);
    if (index >= 0) {
        values[index].selectionValue = selectionValue;
    } else {
        values.push_back(Value(selection, selectionValue));
    }
}

int SceneSelectionPresetCatalog::Slot::value(
    const SceneOptionSelection& selection) const {
    int index = indexOf(selection);
    return (index >= 0) ? values[index].selectionValue : 0;
}

void SceneSelectionPresetCatalog::Slot::saveCurrentValues(
    const SceneSelectionRegistry& registry) {
    for (int i = 0; i < registry.count(); i++) {
        SceneOptionSelection* selection = registry.selectionAt(i);
        if (selection == 0)
            continue;

        setValue(*selection, selection->currentValue());
    }
}

void SceneSelectionPresetCatalog::Slot::restoreValues(
    const SceneSelectionRegistry& registry) const {
    for (int i = 0; i < registry.count(); i++) {
        SceneOptionSelection* selection = registry.selectionAt(i);
        if (selection != 0)
            selection->setValue(value(*selection));
    }
}

SceneSelectionPresetCatalog::SceneSelectionPresetCatalog(
    SceneSelectionRegistry& registry_)
    : registry(registry_) { }

int SceneSelectionPresetCatalog::validSlot(int slot) const {
    return (slot >= 0) && (slot < PresetSlotCount);
}

void SceneSelectionPresetCatalog::save(int slot) {
    if (!validSlot(slot))
        return;

    slots[slot].saveCurrentValues(registry);
}

void SceneSelectionPresetCatalog::restore(int slot) {
    if (!validSlot(slot))
        return;

    registry.saveAll();
    slots[slot].restoreValues(registry);
}

void SceneSelectionPresetCatalog::setValue(
    int slot, SceneOptionSelection& selection, int selectionValue) {
    if (!validSlot(slot))
        return;

    slots[slot].setValue(selection, selectionValue);
}

int SceneSelectionPresetCatalog::value(
    int slot, const SceneOptionSelection& selection) const {
    if (!validSlot(slot))
        return 0;

    return slots[slot].value(selection);
}

SceneSelectionPolicyApplier::SceneSelectionPolicyApplier(
    SceneSelectionRegistry& registry_, SceneSelectionPresetCatalog& presets_)
    : registry(registry_)
    , presets(presets_)
    , policyConfigured(0)
    , allowedChoices()
    , presetPolicies() { }

SceneSelectionPolicyApplier::~SceneSelectionPolicyApplier() { }

void SceneSelectionPolicyApplier::configure(const EffectPolicy& policy) {
    allowedChoices = policy.allowedChoices;
    presetPolicies = policy.presets;
    policyConfigured = 1;

    applyAll();
}

void SceneSelectionPolicyApplier::observe(SceneOptionSelection& selection) {
    if (!policyConfigured)
        return;

    applyAllowedChoicePolicy(selection, allowedChoices);
    applyPresetPolicy(selection, presetPolicies, presets);
}

void SceneSelectionPolicyApplier::applyAll() {
    if (!policyConfigured)
        return;

    for (int i = 0; i < registry.count(); i++) {
        SceneOptionSelection* selection = registry.selectionAt(i);
        if (selection != 0)
            observe(*selection);
    }
}
