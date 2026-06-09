/** @file
 * Unit coverage for FrameFilterchain injected diagnostics.
 */

#include "FrameFilterchain.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

class RecordingLogSink : public LogSink {
public:
    int debugWrites;
    int traceWrites;
    std::string lastDebug;

    RecordingLogSink()
        : debugWrites(0)
        , traceWrites(0)
        , lastDebug() { }

    virtual int enabled(int level) const {
        return level <= CTH_LOG_TRACE;
    }

protected:
    virtual void write(int level, const char*, int, const char* fmt, va_list ap) {
        if (level == CTH_LOG_DEBUG) {
            char message[512];
            vsnprintf(message, sizeof(message), fmt, ap);
            lastDebug = message;
            debugWrites++;
        }
        if (level == CTH_LOG_TRACE)
            traceWrites++;
    }
};

class NoOpFrameFilter : public FrameFilter {
public:
    virtual const char* name() const { return "test-filter"; }

    virtual void execute(FrameFilterFrame&) { }
};

static void testFrameFilterchainUsesInjectedLogSink() {
    RecordingLogSink log;
    FrameFilterchain filterchain(log);
    NoOpFrameFilter filter;

    filterchain.add(7, &filter);
    assert(log.debugWrites == 1);
    assert(log.lastDebug.find("filter=test-filter") != std::string::npos);

    std::vector<unsigned int> stages;
    stages.push_back(7);
    filterchain.setStageSequence(stages);
    assert(log.debugWrites == 2);

    assert(filterchain.setStageMode(7, FrameFilterEnabled) == 1);
    assert(log.traceWrites == 1);
}

int main() {
    testFrameFilterchainUsesInjectedLogSink();
    return 0;
}
