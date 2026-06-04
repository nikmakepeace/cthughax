// Lazy startup policy resolver for registered EffectControls.

#ifndef CTHUGHA_EFFECT_CONTROL_POLICY_H
#define CTHUGHA_EFFECT_CONTROL_POLICY_H

class EffectControl;
struct EffectPolicy;

/** Installs startup policy and applies it to currently registered controls. */
void configureEffectPolicy(const EffectPolicy& policy);

/** Rechecks startup policy for a control whose choices may have changed. */
void effectControlPolicyObserve(EffectControl& option);

#endif
