// Ini-file adapter for CoreOption values, usage flags, and hot slots.

#ifndef __CORE_OPTION_INI_H
#define __CORE_OPTION_INI_H

class CoreOption;

/** Reads initial CoreOption selections from the currently open ini source. */
void coreOptionGetIniInitials();

/** Writes current CoreOption selections to the currently open ini output. */
void coreOptionPutIniInitials();

/** Reads per-entry CoreOption usage flags from the currently open ini source. */
void coreOptionGetIniUsages();

/** Writes per-entry CoreOption usage flags to the currently open ini output. */
void coreOptionPutIniUsages();

/** Reads CoreOption hot-slot selections from the currently open ini source. */
void coreOptionGetHotIni();

/** Writes CoreOption hot-slot selections to the currently open ini output. */
void coreOptionPutHotIni();

/**
 * Checks whether an ini key belongs to any registered CoreOption.
 *
 * @param entry Ini key without the "cthugha." prefix.
 * @return Nonzero when the key is a CoreOption initial value, usage flag, hot
 *         slot, or legacy wildcard key.
 */
int coreOptionIsIniEntry(const char* entry);

#endif
