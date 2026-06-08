// Legacy wave option declarations.

#ifndef CTHUGHA_WAVE_OPTIONS_H
#define CTHUGHA_WAVE_OPTIONS_H

#include "EffectControl.h"

class Wave;
struct EffectPolicy;

/**
 * Effect choice that points at one built-in Wave catalog item.
 */
class WaveEntry : public EffectChoice {
    Wave* waveValue;

public:
    /**
     * Creates an option entry for a wave.
     *
     * @param wave Borrowed wave catalog entry.
     * @param inUse Nonzero when the option should be selectable by default.
     */
    WaveEntry(Wave& wave, int inUse = 1);

    /** @return Borrowed wave catalog entry. */
    Wave& wave() const;
};

/**
 * Option wrapper for selecting the current wave renderer.
 */
class WaveOption : public EffectControl {
public:
    WaveOption();

    /** @return Currently selected wave, or NULL. */
    Wave* currentWave();
};

/** Global wave renderer selection option. */
extern WaveOption wave;

/** Wave-scale option; interpretation is wave-specific. */
extern EffectControl waveScale;

/** Palette color-table option for wave drawing. */
extern EffectControl table;

/** 3D object selection option for object-capable waves. */
extern EffectControl object;

/** Enables loading/selecting 3D object files for object-capable waves. */
extern OptionOnOff use_objects;

/** Applies startup policy for legacy object loading. */
void configureWaveOptions(const EffectPolicy& config);

#endif
