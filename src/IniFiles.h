/** @file
 * Runtime ini persistence and continuation ini adapters.
 */

#ifndef CTHUGHA_INI_FILES_H
#define CTHUGHA_INI_FILES_H

#include "Configuration.h"

class LogSink;

/**
 * Stop-and-continue snapshot written to the continuation ini.
 */
struct ContinuationIniConfig {
    /** Runtime scene selections to restore on the next startup. */
    SceneConfig scene;

    /** Nonzero when the FPS overlay should be restored. */
    int showFpsEnabled;

    /** Creates an empty continuation snapshot. */
    ContinuationIniConfig();
};

/**
 * Removes the pending continuation ini file, if one was discovered at startup.
 *
 * @param paths Startup path configuration containing the continuation file.
 * @param log Logging sink for removal diagnostics.
 * @return Zero on success; non-zero only for unrecoverable errors.
 */
int remove_continuation_ini(const PathConfig& paths, LogSink& log);

/**
 * Writes the automatic ini file from an explicit configuration snapshot.
 *
 * @param config Configuration snapshot to serialize.
 * @param log Logging sink for write/install diagnostics.
 * @return Zero on success, non-zero on write/install failure.
 */
int write_ini(const Config& config, LogSink& log);

/**
 * Writes the stop-and-continue ini file from runtime continuation state.
 *
 * @param config Continuation snapshot to serialize.
 * @param log Logging sink for write/install diagnostics.
 * @return Zero on success, non-zero on write/install failure.
 */
int write_continuation_ini(const ContinuationIniConfig& config, LogSink& log);

#endif
