/** @file
 * Production process-service adapters.
 */

#include "ProcessServices.h"

#include <chrono>

#include "Configuration.h"
#include "defaults.h"

#include <stdio.h>
#include <string.h>

static double steadyClockSeconds() {
    std::chrono::steady_clock::duration elapsed
        = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(elapsed).count();
}

LoggingRuntime::LoggingRuntime()
    : verbosityValue(DEFAULT_VERBOSE_MIN_LEVEL) {
}

void LoggingRuntime::configure(const LoggingConfig& config) {
    configureVerbosity(config.verbosity);
}

void LoggingRuntime::configureVerbosity(int verbosity) {
    verbosityValue = verbosity;
    if (verbosityValue < DEFAULT_VERBOSE_MIN_LEVEL)
        verbosityValue = DEFAULT_VERBOSE_MIN_LEVEL;
}

int LoggingRuntime::verbosity() const {
    return verbosityValue;
}

int LoggingRuntime::enabled(int level) const {
    return (level <= CTH_LOG_ERROR) || (level <= verbosityValue);
}

static void copyConsoleFormat(char* out, const char* in) {
    int i;
    for (i = 0; *in != '\0'; in++) {
        out[i++] = *in;
        if (*in == '\n')
            out[i++] = '\r';
    }
    out[i] = '\0';
}

static void printContextPrefix(FILE* stream, const char* context) {
    if ((context != NULL) && (context[0] != '\0'))
        fprintf(stream, "%s: ", context);
}

int LogSink::debugEnabled() const {
    return enabled(CTH_LOG_DEBUG);
}

int LogSink::traceEnabled() const {
    return enabled(CTH_LOG_TRACE);
}

void LogSink::debug(const char* fmt, ...) {
    if (!debugEnabled())
        return;

    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_DEBUG, NULL, -1, fmt, ap);
    va_end(ap);
}

void LogSink::info(const char* fmt, ...) {
    if (!enabled(CTH_LOG_INFO))
        return;

    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_INFO, NULL, -1, fmt, ap);
    va_end(ap);
}

void LogSink::warn(const char* fmt, ...) {
    if (!enabled(CTH_LOG_WARN))
        return;

    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_WARN, NULL, -1, fmt, ap);
    va_end(ap);
}

void LogSink::error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_ERROR, NULL, -1, fmt, ap);
    va_end(ap);
}

void LogSink::errorErrno(int errnum, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_ERROR, NULL, errnum, fmt, ap);
    va_end(ap);
}

void LogSink::trace(const char* context, const char* fmt, ...) {
    if (!traceEnabled())
        return;

    va_list ap;
    va_start(ap, fmt);
    write(CTH_LOG_TRACE, context, -1, fmt, ap);
    va_end(ap);
}

ConsoleLogSink::ConsoleLogSink(LoggingRuntime& runtime)
    : runtimeValue(runtime) {
}

int ConsoleLogSink::enabled(int level) const {
    return runtimeValue.enabled(level);
}

void ConsoleLogSink::write(int level, const char* context, int errnum,
    const char* fmt, va_list ap) {
    char fmtWithReturns[2 * strlen(fmt) + 1];
    copyConsoleFormat(fmtWithReturns, fmt);

    if (level <= CTH_LOG_ERROR) {
        printf("\n\r");
        fflush(stdout);

        printContextPrefix(stderr, context);
#ifdef HAVE_VPRINTF
        vfprintf(stderr, fmtWithReturns, ap);
#else
        fprintf(stderr, "%s", fmtWithReturns);
#endif
        if (errnum >= 0) {
            fprintf(stderr, " (%d - %s)\n\r", errnum, strerror(errnum));
        }
        fflush(stderr);
        return;
    }

    printContextPrefix(stdout, context);
#ifdef HAVE_VPRINTF
    vprintf(fmtWithReturns, ap);
#else
    printf("%s", fmtWithReturns);
#endif
    if (level <= CTH_LOG_INFO)
        fflush(stdout);
}

SystemMillisecondClock::SystemMillisecondClock()
    : startSecondsValue(steadyClockSeconds()) {
}

int SystemMillisecondClock::milliseconds() const {
    return int((steadyClockSeconds() - startSecondsValue) * 1000.0);
}

double SystemSecondsClock::nowSeconds() const {
    return steadyClockSeconds();
}

SystemCountdownTimer::SystemCountdownTimer(int durationMs)
    : deadlineSecondsValue(steadyClockSeconds()
          + (durationMs > 0 ? double(durationMs) / 1000.0 : 0.0)) {
}

int SystemCountdownTimer::millisecondsRemaining() const {
    double remainingSeconds = deadlineSecondsValue - steadyClockSeconds();
    if (remainingSeconds <= 0.0)
        return 0;

    return int(remainingSeconds * 1000.0);
}

std::unique_ptr<CountdownTimer> SystemCountdownTimerFactory::startTimer(
    int durationMs) const {
    return std::unique_ptr<CountdownTimer>(new SystemCountdownTimer(durationMs));
}

CStdRandomSource::CStdRandomSource()
    : stateValue((unsigned int)time(NULL)) {
    if (stateValue == 0)
        stateValue = 1;
}

CStdRandomSource::CStdRandomSource(unsigned int seed_)
    : stateValue(seed_ != 0 ? seed_ : 1) {
}

int CStdRandomSource::uniformInt(int exclusiveMax) {
    if (exclusiveMax <= 1)
        return 0;

    stateValue = stateValue * 1103515245u + 12345u;
    return int((stateValue >> 16) % (unsigned int)exclusiveMax);
}
