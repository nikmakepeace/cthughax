#ifndef CTHUGHA_DISPLAY_PRESENTATION_OPTIONS_H
#define CTHUGHA_DISPLAY_PRESENTATION_OPTIONS_H

#include "Option.h"

struct DisplayConfig;

/**
 * Transitional presentation options owned by the Display module.
 *
 * The values are still Option-backed so existing interface and command
 * behavior remains compatible, but ownership is now explicit through
 * DisplaySystem instead of process-wide globals.
 */
class DisplayPresentationSettings {
public:
    OptionInt maxFramesPerSecond;
    OptionOnOff showFPS;
    OptionInt zoom;

    /** Creates presentation settings with default option names and values. */
    DisplayPresentationSettings();

    /** Applies startup display configuration to the live presentation state. */
    void configure(const DisplayConfig& config);
};

#endif
