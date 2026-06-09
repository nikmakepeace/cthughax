/** @file
 * Numeric log levels shared by explicit logging sinks and legacy adapters.
 */

#ifndef CTHUGHA_LOG_LEVELS_H
#define CTHUGHA_LOG_LEVELS_H

/** Logging severity/verbosity levels used by Cthugha diagnostics. */
enum CthughaLogLevel {
    /** Error diagnostics are always emitted. */
    CTH_LOG_ERROR = 0,

    /** Warning diagnostics. */
    CTH_LOG_WARN = 1,

    /** Informational startup/runtime diagnostics. */
    CTH_LOG_INFO = 2,

    /** Debug diagnostics. */
    CTH_LOG_DEBUG = 5,

    /** High-volume trace diagnostics. */
    CTH_LOG_TRACE = 10
};

#endif
