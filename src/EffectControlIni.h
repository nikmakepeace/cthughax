// Ini-file adapter for EffectControl values, usage flags, and preset slots.

#ifndef __EFFECT_CONTROL_INI_H
#define __EFFECT_CONTROL_INI_H

class EffectControl;

/** Writes current EffectControl selections to the currently open ini output. */
void effectControlPutIniInitials();

/** Writes per-entry EffectControl usage flags to the currently open ini output. */
void effectControlPutIniUsages();

/** Writes EffectControl preset-slot selections to the currently open ini output. */
void effectControlPutPresetIni();

#endif
