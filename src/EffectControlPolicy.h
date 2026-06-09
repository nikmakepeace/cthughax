// Lazy startup policy resolver for registered EffectControls.

#ifndef CTHUGHA_EFFECT_CONTROL_POLICY_H
#define CTHUGHA_EFFECT_CONTROL_POLICY_H

#include "Configuration.h"

#include <vector>

class EffectControl;
class EffectPresetCatalog;
class EffectRegistry;
struct EffectPolicy;

/** Applies startup effect policy to an explicit registry and preset catalog. */
class EffectPolicyApplier {
    EffectRegistry& registry;
    EffectPresetCatalog& presets;
    int policyConfigured;
    std::vector<EffectChoicePolicy> allowedChoices;
    std::vector<EffectPresetPolicy> presetPolicies;

public:
    EffectPolicyApplier(EffectRegistry& registry_,
        EffectPresetCatalog& presets_);
    ~EffectPolicyApplier();

    /** Installs startup policy and applies it to registered controls. */
    void configure(const EffectPolicy& policy);

    /** Rechecks startup policy for a control whose choices may have changed. */
    void observe(EffectControl& option);

    /** Applies configured policy to every registered control. */
    void applyAll();
};

#endif
