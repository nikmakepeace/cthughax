/** @file
 * Unit coverage for Application-owned process service adapters.
 */

#include "ProcessServices.h"

#include "cthugha.h"
#include "defaults.h"

#include <assert.h>

class RecordingLogSink : public LogSink {
    int enabledThroughValue;

protected:
    virtual void write(int level, const char* context, int errnum,
        const char*, va_list) {
        writes++;
        lastLevel = level;
        lastContext = context;
        lastErrno = errnum;
    }

public:
    int writes;
    int lastLevel;
    const char* lastContext;
    int lastErrno;

    explicit RecordingLogSink(int enabledThrough)
        : enabledThroughValue(enabledThrough)
        , writes(0)
        , lastLevel(-1)
        , lastContext(0)
        , lastErrno(-1) { }

    virtual int enabled(int level) const {
        return level <= enabledThroughValue;
    }
};

static void testLoggingRuntimeClampsVerbosity() {
    LoggingRuntime logging;

    logging.configureVerbosity(DEFAULT_VERBOSE_MIN_LEVEL - 10);

    assert(logging.verbosity() == DEFAULT_VERBOSE_MIN_LEVEL);
}

static void testLoggingRuntimeEnablesExpectedLevels() {
    LoggingRuntime logging;

    logging.configureVerbosity(CTH_LOG_DEBUG);

    assert(logging.enabled(CTH_LOG_ERROR));
    assert(logging.enabled(CTH_LOG_WARN));
    assert(logging.enabled(CTH_LOG_DEBUG));
    assert(!logging.enabled(CTH_LOG_TRACE));
}

static void testConsoleLogSinkUsesLoggingRuntime() {
    LoggingRuntime logging;
    logging.configureVerbosity(CTH_LOG_WARN);
    ConsoleLogSink sink(logging);

    assert(sink.enabled(CTH_LOG_ERROR));
    assert(sink.enabled(CTH_LOG_WARN));
    assert(!sink.enabled(CTH_LOG_DEBUG));
}

static void testLogSinkConvenienceMethodsFilterThroughEnabledLevels() {
    RecordingLogSink sink(CTH_LOG_WARN);

    sink.debug("debug %d", 1);
    assert(sink.writes == 0);

    sink.warn("warning %d", 2);
    assert(sink.writes == 1);
    assert(sink.lastLevel == CTH_LOG_WARN);
    assert(sink.lastContext == 0);
    assert(sink.lastErrno == -1);

    sink.trace("process", "trace %d", 3);
    assert(sink.writes == 1);

    sink.errorErrno(17, "error");
    assert(sink.writes == 2);
    assert(sink.lastLevel == CTH_LOG_ERROR);
    assert(sink.lastErrno == 17);
}

static void testSmallRandomRangesReturnZero() {
    CStdRandomSource randomSource(1234);

    assert(randomSource.uniformInt(0) == 0);
    assert(randomSource.uniformInt(1) == 0);
}

static void testSeededRandomSourcesAreDeterministicAndIndependent() {
    CStdRandomSource first(42);
    CStdRandomSource second(42);

    int firstValue = first.uniformInt(1000);
    int secondValue = second.uniformInt(1000);
    assert(firstValue == secondValue);

    (void)first.uniformInt(1000);

    CStdRandomSource expectedSecond(42);
    (void)expectedSecond.uniformInt(1000);
    assert(second.uniformInt(1000) == expectedSecond.uniformInt(1000));
}

static void testSystemMillisecondClockIsMonotonic() {
    SystemMillisecondClock clock;

    int first = clock.milliseconds();
    int second = clock.milliseconds();

    assert(first >= 0);
    assert(second >= first);
}

static void testSystemSecondsClockIsMonotonic() {
    SystemSecondsClock clock;

    double first = clock.nowSeconds();
    double second = clock.nowSeconds();

    assert(first >= 0.0);
    assert(second >= first);
}

static void testSystemCountdownTimerExpiresImmediateDurations() {
    SystemCountdownTimer zeroTimer(0);
    SystemCountdownTimer negativeTimer(-10);

    assert(zeroTimer.millisecondsRemaining() == 0);
    assert(negativeTimer.millisecondsRemaining() == 0);
}

static void testSystemCountdownTimerFactoryCreatesTimer() {
    SystemCountdownTimerFactory factory;
    std::unique_ptr<CountdownTimer> timer = factory.startTimer(1000);

    assert(timer.get() != 0);
    assert(timer->millisecondsRemaining() > 0);
    assert(timer->millisecondsRemaining() <= 1000);
}

int main() {
    testLoggingRuntimeClampsVerbosity();
    testLoggingRuntimeEnablesExpectedLevels();
    testConsoleLogSinkUsesLoggingRuntime();
    testLogSinkConvenienceMethodsFilterThroughEnabledLevels();
    testSmallRandomRangesReturnZero();
    testSeededRandomSourcesAreDeterministicAndIndependent();
    testSystemMillisecondClockIsMonotonic();
    testSystemSecondsClockIsMonotonic();
    testSystemCountdownTimerExpiresImmediateDurations();
    testSystemCountdownTimerFactoryCreatesTimer();
    return 0;
}
