// Scene runtime wiring ports and adapters.

#ifndef CTHUGHA_SCENE_RUNTIME_DEPENDENCIES_H
#define CTHUGHA_SCENE_RUNTIME_DEPENDENCIES_H

#include "Configuration.h"
#include "SceneDependencies.h"

#include <memory>
#include <vector>

struct EffectPolicy;
class SceneOptionSelection;
class SceneVisualSelections;

class SceneVisualCatalogFactoryResult {
public:
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs;
    SceneVisualSelections* selections;

    SceneVisualCatalogFactoryResult();
    SceneVisualCatalogFactoryResult(
        std::unique_ptr<SceneVisualCatalogs> visualCatalogs_,
        SceneVisualSelections& selections_);
};

class SceneVisualCatalogFactory {
public:
    virtual ~SceneVisualCatalogFactory();
    virtual SceneVisualCatalogFactoryResult create(
        SceneSelectionState& selectionState) = 0;
};

/**
 * Registry of Scene-owned visual selections used by SceneCommands.
 */
class SceneSelectionRegistry : public SceneEffectRegistry {
    struct Entry {
        SceneOptionSelection* selection;
        std::vector<int> history;

        explicit Entry(SceneOptionSelection& selection_);
    };

    std::vector<Entry> entries;

    int indexOf(const SceneOptionSelection& selection) const;
    void saveEntry(Entry& entry);
    void restoreEntry(Entry& entry);

public:
    SceneSelectionRegistry();

    void registerSelection(SceneOptionSelection& selection);
    void registerSelections(SceneVisualSelections& selections);

    int count() const;
    SceneOptionSelection* selectionAt(int index) const;

    virtual void saveAll();
    virtual void restoreAll();
    virtual void changeAll(RandomSource& randomSource);
    virtual void changeOne(RandomSource& randomSource);
};

/**
 * Preset slots over Scene-owned visual selections.
 */
class SceneSelectionPresetCatalog : public ScenePresetCatalog {
    struct Value {
        SceneOptionSelection* selection;
        int selectionValue;

        Value(SceneOptionSelection& selection_, int selectionValue_);
    };

    class Slot {
        std::vector<Value> values;

        int indexOf(const SceneOptionSelection& selection) const;

    public:
        void setValue(SceneOptionSelection& selection, int selectionValue);
        int value(const SceneOptionSelection& selection) const;
        void saveCurrentValues(const SceneSelectionRegistry& registry);
        void restoreValues(const SceneSelectionRegistry& registry) const;
    };

    static const int PresetSlotCount = 10;

    SceneSelectionRegistry& registry;
    Slot slots[PresetSlotCount];

    int validSlot(int slot) const;

public:
    explicit SceneSelectionPresetCatalog(SceneSelectionRegistry& registry_);

    virtual void save(int slot);
    virtual void restore(int slot);
    void setValue(int slot, SceneOptionSelection& selection, int selectionValue);
    int value(int slot, const SceneOptionSelection& selection) const;
};

/** Applies startup effect policy to Scene-owned visual selections. */
class SceneSelectionPolicyApplier {
    SceneSelectionRegistry& registry;
    SceneSelectionPresetCatalog& presets;
    int policyConfigured;
    std::vector<EffectChoicePolicy> allowedChoices;
    std::vector<EffectPresetPolicy> presetPolicies;

public:
    SceneSelectionPolicyApplier(SceneSelectionRegistry& registry_,
        SceneSelectionPresetCatalog& presets_);
    ~SceneSelectionPolicyApplier();

    void configure(const EffectPolicy& policy);
    void observe(SceneOptionSelection& selection);
    void applyAll();
};

#endif
