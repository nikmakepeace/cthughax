// Preset slots for whole-effect snapshots.

#ifndef __EFFECT_PRESET_CATALOG_H
#define __EFFECT_PRESET_CATALOG_H

#include <vector>

class EffectControl;
class EffectRegistry;

class EffectPresetSlot {
    struct Value {
        EffectControl* option;
        int selection;

        Value(EffectControl* option_, int selection_)
            : option(option_)
            , selection(selection_) { }
    };

    std::vector<Value> values;

    int indexOf(const EffectControl& option) const;

public:
    void setValue(EffectControl& option, int selection);
    int value(const EffectControl& option) const;
    void saveCurrentValues(const EffectRegistry& registry);
    void restoreValues(const EffectRegistry& registry);
};

class EffectPresetCatalog {
    static const int PresetSlotCount = 10;

    EffectRegistry& registry;
    EffectPresetSlot slots[PresetSlotCount];

    int validSlot(int slot) const;

public:
    explicit EffectPresetCatalog(EffectRegistry& registry_);
    virtual ~EffectPresetCatalog();

    int slotCount() const;

    virtual void save(int slot);
    virtual void restore(int slot);
    virtual void setValue(int slot, EffectControl& option, int selection);
    virtual int value(int slot, const EffectControl& option) const;
};

#endif
