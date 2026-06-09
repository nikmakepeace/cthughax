// Explicit registry for selectable effect controls.

#ifndef CTHUGHA_EFFECT_REGISTRY_H
#define CTHUGHA_EFFECT_REGISTRY_H

#include <vector>

class EffectControl;
class RandomSource;

/**
 * Owns explicit iteration over effect controls that participate in scene
 * selection, presets, policy, and automatic changes.
 */
class EffectRegistry {
    std::vector<EffectControl*> controls;

    int indexOf(const EffectControl& control) const;

public:
    /** Removes all registered controls. */
    void clear();

    /** Adds a control if it is not already registered. */
    void registerControl(EffectControl& control);

    /** @return Number of registered controls. */
    int count() const;

    /** @return Registered control at index, or NULL when out of range. */
    EffectControl* controlAt(int index) const;

    /** Saves all registered control selections. */
    void saveAll();

    /** Restores all registered control selections. */
    void restoreAll();

    /** Randomly changes every eligible registered auto-change control. */
    void changeAll(RandomSource& randomSource);

    /**
     * Randomly changes one eligible registered auto-change control.
     *
     * @return Changed control, or NULL when no unlocked candidate changes.
     */
    EffectControl* changeOne(RandomSource& randomSource);
};

#endif
