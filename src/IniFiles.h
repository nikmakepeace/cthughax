/** @file
 * Runtime ini persistence and continuation ini adapters.
 */

#ifndef CTHUGHA_INI_FILES_H
#define CTHUGHA_INI_FILES_H

#include "Configuration.h"

struct ContinuationIniConfig {
    SceneConfig scene;
    int showFpsEnabled;

    /** Creates an empty continuation snapshot. */
    ContinuationIniConfig();
};

/**
 * Removes the pending continuation ini file, if one was discovered at startup.
 *
 * @param paths Startup path configuration containing the continuation file.
 * @return Zero on success; non-zero only for unrecoverable errors.
 */
int remove_continuation_ini(const PathConfig& paths);

/**
 * Writes the automatic ini file from an explicit configuration snapshot.
 *
 * @param config Configuration snapshot to serialize.
 * @return Zero on success, non-zero on write/install failure.
 */
int write_ini(const Config& config);

/**
 * Writes the stop-and-continue ini file from runtime continuation state.
 *
 * @param config Continuation snapshot to serialize.
 * @return Zero on success, non-zero on write/install failure.
 */
int write_continuation_ini(const ContinuationIniConfig& config);

#endif
