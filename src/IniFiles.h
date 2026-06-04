// Runtime ini persistence and effect-control ini adapters.

#ifndef CTHUGHA_INI_FILES_H
#define CTHUGHA_INI_FILES_H

#include "Configuration.h"
#include "EffectControl.h"
#include "Option.h"

int remove_continuation_ini(const PathConfig& paths);
int write_ini();
int write_continuation_ini();

int getini(const char* entry, char* value);
int getini(const char* entry, int* value);
int getini_yesno(const char* entry, int* value);

int putini(const char* entry, const char* value);
int putini(const Option& opt);

#endif
