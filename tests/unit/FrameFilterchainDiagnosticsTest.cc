/** @file
 * Unit coverage for FrameFilterchain injected diagnostics.
 */

#include "FrameFilterchain.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

class RecordingLogSink : public LogSink {
public:
    int debugWrites;
    int traceWrites;

    RecordingLogSink()
        : debugWrites(0)
        , traceWrites(0) { }

    virtual int enabled(int level) const {
        return level <= CTH_LOG_TRACE;
    }

protected:
    virtual void write(int level, const char*, int, const char*, va_list) {
        if (level == CTH_LOG_DEBUG)
            debugWrites++;
        if (level == CTH_LOG_TRACE)
            traceWrites++;
    }
};

class NoOpFrameFilter : public FrameFilter {
public:
    virtual void execute(FrameFilterFrame&) { }
};

static void testFrameFilterchainUsesInjectedLogSink() {
    RecordingLogSink log;
    FrameFilterchain filterchain(log);
    NoOpFrameFilter filter;

    filterchain.add(7, &filter);
    assert(log.debugWrites == 1);

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
