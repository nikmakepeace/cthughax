// Legacy border option declaration.

#ifndef CTHUGHA_BORDER_OPTION_H
#define CTHUGHA_BORDER_OPTION_H

#include "EffectControl.h"

/** Global border drawing option used by legacy command/config bridges. */
extern EffectControl border;

/** Initializes the built-in border option catalog. */
void init_border();

#endif
