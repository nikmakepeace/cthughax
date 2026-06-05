/** @file
 * Automatic scene-change policy driven by audio metrics and acoustic context.
 */

#ifndef __AUTO_CHANGER_H
#define __AUTO_CHANGER_H

#include "Option.h"

extern OptionTime changeQuiet; /* change after quiet-pause (1.5 sec) */
extern OptionTime changeWaitMin; /* min time between change (3 sec) */
extern OptionTime changeWaitRandom; /* extra random wait-time (11 sec) */
extern OptionInt changeCumulativeFireLevel;
extern OptionOnOff lock; /* change automatically */
extern OptionOnOff change_little; /* only change one options */

class RuntimeCommandSink;
class AcousticContext;
struct AutoChangeConfig;
struct AudioMetrics;

class AutoChanger {
    RuntimeCommandSink& runtimeCommands;
    AcousticContext& acousticContextValue;

    int quietSince;
    int waitTime;
    int lastChange;

public:
    /**
     * Creates the automatic scene changer.
     *
     * @param runtimeCommands_ Runtime command sink used for automatic scene
     *        mutations. The referenced object must outlive this AutoChanger.
     * @param acousticContext_ Rolling acoustic state used for fire-triggered
     *        scene changes. The referenced object must outlive this AutoChanger.
     */
    AutoChanger(RuntimeCommandSink& runtimeCommands_,
        AcousticContext& acousticContext_);

    /** Releases automatic scene-change policy state. */
    ~AutoChanger();

    /**
     * Runs one automatic-change policy step using supplied audio metrics.
     *
     * @param metrics Metrics for the current visual audio frame.
     */
    void operator()(const AudioMetrics& metrics);

    /**
     * Applies the selected automatic change action.
     *
     * Uses the little option to choose between changing one eligible EffectControl
     * and changing the whole unlocked scene option set. Exact entry selection is
     * still owned by the runtime command handler and legacy EffectControl policy.
     */
    void change();

    /**
     * @return Pointer to static status text for the interface. The text is
     *         overwritten on the next status() call.
     */
    const char* status() const;
};

/**
 * Applies startup automatic scene-change configuration to legacy options.
 *
 * @param config Startup configuration values parsed before Application creates
 *        the bridge-owned AutoChanger instance.
 */
void configureAutoChanger(const AutoChangeConfig& config);

#endif
