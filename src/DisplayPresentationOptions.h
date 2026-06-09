#ifndef CTHUGHA_DISPLAY_PRESENTATION_OPTIONS_H
#define CTHUGHA_DISPLAY_PRESENTATION_OPTIONS_H

#include "Option.h"

/**
 * Transitional presentation options owned by the Display module.
 *
 * These globals are kept behind a narrow display header while the remaining
 * Option-based runtime controls move to object-owned DisplaySystem settings.
 */
extern OptionInt zoom;
extern OptionInt maxFramesPerSecond;
extern OptionOnOff showFPS;

#endif
