/** @file
 * Small process-service interfaces owned by the application lifecycle.
 */

#ifndef CTHUGHA_PROCESS_SERVICES_H
#define CTHUGHA_PROCESS_SERVICES_H

#include "LogLevels.h"

#include <memory>
#include <stdarg.h>

struct LoggingConfig;

/**
 * Mutable logging level owned by the application lifecycle.
 */
class LoggingRuntime {
    int verbosityValue;

public:
    /** Creates logging state at the minimum startup verbosity. */
    LoggingRuntime();

    /**
     * Applies startup logging configuration.
     *
     * @param config Final logging configuration produced by startup config.
     */
    void configure(const LoggingConfig& config);

    /**
     * Applies a raw verbosity value.
     *
     * @param verbosity Requested verbosity before minimum-level clamping.
     */
    void configureVerbosity(int verbosity);

    /** @return Current effective logging verbosity. */
    int verbosity() const;

    /**
     * Checks whether a log level is enabled.
     *
     * @param level Cthugha log level to test.
     * @return Nonzero when the level should be emitted.
     */
    int enabled(int level) const;
};

/**
 * Narrow logging sink passed to modules that should not reach through CTH_*.
 */
class LogSink {
public:
    /** Destroys the logging sink interface. */
    virtual ~LogSink() { }

    /**
     * Checks whether a raw log level is enabled.
     *
     * @param level Cthugha numeric log level to test.
     * @return Nonzero when the level should be emitted.
     */
    virtual int enabled(int level) const = 0;

    /** @return Nonzero when debug-level diagnostics should be emitted. */
    int debugEnabled() const;

    /** @return Nonzero when trace-level diagnostics should be emitted. */
    int traceEnabled() const;

    /**
     * Emits a debug-level message when debug logging is enabled.
     *
     * @param fmt printf-style format string.
     */
    void debug(const char* fmt, ...);

    /**
     * Emits an info-level message when info logging is enabled.
     *
     * @param fmt printf-style format string.
     */
    void info(const char* fmt, ...);

    /**
     * Emits a warning-level message when warning logging is enabled.
     *
     * @param fmt printf-style format string.
     */
    void warn(const char* fmt, ...);

    /**
     * Emits an error-level message.
     *
     * @param fmt printf-style format string.
     */
    void error(const char* fmt, ...);

    /**
     * Emits an error-level message with an errno suffix.
     *
     * @param errnum errno value to append.
     * @param fmt printf-style format string.
     */
    void errorErrno(int errnum, const char* fmt, ...);

    /**
     * Emits a trace-level message with context when trace logging is enabled.
     *
     * @param context Short subsystem/context label.
     * @param fmt printf-style format string.
     */
    void trace(const char* context, const char* fmt, ...);

protected:
    /**
     * Writes an already-enabled log message.
     *
     * @param level Cthugha numeric log level.
     * @param context Optional context label, or NULL.
     * @param errnum errno value to append, or -1 when absent.
     * @param fmt printf-style format string.
     * @param ap printf argument list.
     */
    virtual void write(int level, const char* context, int errnum,
        const char* fmt, va_list ap) = 0;
};

/**
 * Console log sink backed by Application-owned LoggingRuntime.
 */
class ConsoleLogSink : public LogSink {
    LoggingRuntime& runtimeValue;

public:
    /**
     * Creates a console sink using the supplied runtime log level.
     *
     * @param runtime Logging runtime owned by Application.
     */
    explicit ConsoleLogSink(LoggingRuntime& runtime);

    /**
     * Checks whether a raw log level is enabled.
     *
     * @param level Cthugha numeric log level to test.
     * @return Nonzero when the level should be emitted.
     */
    virtual int enabled(int level) const;

protected:
    /**
     * Writes an already-enabled log message to stdout/stderr.
     *
     * @param level Cthugha numeric log level.
     * @param context Optional context label, or NULL.
     * @param errnum errno value to append, or -1 when absent.
     * @param fmt printf-style format string.
     * @param ap printf argument list.
     */
    virtual void write(int level, const char* context, int errnum,
        const char* fmt, va_list ap);
};

/**
 * Millisecond clock used by runtime policy code.
 */
class MillisecondClock {
public:
    /** Destroys the clock interface. */
    virtual ~MillisecondClock() { }

    /** @return Current process/application time in milliseconds. */
    virtual int milliseconds() const = 0;
};

/**
 * Production millisecond clock with an owned steady-clock start point.
 */
class SystemMillisecondClock : public MillisecondClock {
    double startSecondsValue;

public:
    /** Creates a clock whose millisecond values are relative to construction. */
    SystemMillisecondClock();

    /** @return Current process/application time in milliseconds. */
    virtual int milliseconds() const;
};

/**
 * Seconds-resolution clock used by frame/audio pacing code.
 */
class SecondsClock {
public:
    /** Destroys the clock interface. */
    virtual ~SecondsClock() { }

    /** @return Current process/application time in seconds. */
    virtual double nowSeconds() const = 0;
};

/**
 * Production seconds clock backed by the process steady clock.
 */
class SystemSecondsClock : public SecondsClock {
public:
    /** @return Current process/application time in seconds. */
    virtual double nowSeconds() const;
};

/**
 * Per-operation countdown timer used by detached or bounded work.
 */
class CountdownTimer {
public:
    /** Destroys the timer interface. */
    virtual ~CountdownTimer() { }

    /**
     * Computes the time remaining before this timer expires.
     *
     * @return Remaining milliseconds, clamped to zero after expiry.
     */
    virtual int millisecondsRemaining() const = 0;
};

/**
 * Factory for per-operation countdown timers.
 */
class CountdownTimerFactory {
public:
    /** Destroys the timer factory interface. */
    virtual ~CountdownTimerFactory() { }

    /**
     * Starts a timer with the requested duration.
     *
     * @param durationMs Timer duration in milliseconds.
     * @return New timer owned by the caller.
     */
    virtual std::unique_ptr<CountdownTimer> startTimer(int durationMs) const = 0;
};

/**
 * Production countdown timer backed by the process steady clock.
 */
class SystemCountdownTimer : public CountdownTimer {
    double deadlineSecondsValue;

public:
    /**
     * Starts a timer that expires after the supplied duration.
     *
     * @param durationMs Timer duration in milliseconds.
     */
    explicit SystemCountdownTimer(int durationMs);

    /** @return Remaining milliseconds, clamped to zero after expiry. */
    virtual int millisecondsRemaining() const;
};

/**
 * Production factory for system countdown timers.
 */
class SystemCountdownTimerFactory : public CountdownTimerFactory {
public:
    /** @return New SystemCountdownTimer owned by the caller. */
    virtual std::unique_ptr<CountdownTimer> startTimer(int durationMs) const;
};

/**
 * Integer random source used by runtime policy code.
 */
class RandomSource {
public:
    /** Destroys the random-source interface. */
    virtual ~RandomSource() { }

    /**
     * Returns a value in [0, exclusiveMax).
     *
     * @param exclusiveMax Upper exclusive bound. Values <= 1 return 0.
     * @return Uniform-ish integer in the requested range.
     */
    virtual int uniformInt(int exclusiveMax) = 0;
};

/**
 * Production random source with owned deterministic state.
 */
class CStdRandomSource : public RandomSource {
    unsigned int stateValue;

public:
    /** Seeds the generator from process startup time. */
    CStdRandomSource();

    /**
     * Creates a deterministic generator for tests or controlled sessions.
     *
     * @param seed_ Initial nonzero seed. Zero is normalized to one.
     */
    explicit CStdRandomSource(unsigned int seed_);

    /** @return Generator-backed value in [0, exclusiveMax), or 0 for small ranges. */
    virtual int uniformInt(int exclusiveMax);
};

/**
 * Installs Application-owned logging state for the legacy C logging adapter.
 *
 * @param runtime Logging state owned by the current application lifecycle.
 */
void cthugha_install_logging_runtime(LoggingRuntime& runtime);

/**
 * Clears the legacy C logging adapter if it points at the given runtime.
 *
 * @param runtime Logging state previously installed by the application.
 */
void cthugha_clear_logging_runtime(LoggingRuntime& runtime);

#endif
