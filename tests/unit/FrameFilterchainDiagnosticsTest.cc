/** @file
 * Unit coverage for FrameFilterchain injected diagnostics.
 */

#include "FrameFilterchain.h"
#include "FrameRenderTarget.h"
#include "FrameStore.h"
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
    std::vector<std::string> traceMessages;

    RecordingLogSink()
        : debugWrites(0)
        , traceWrites(0)
        , lastDebug()
        , traceMessages() { }

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
        if (level == CTH_LOG_TRACE) {
            char message[512];
            vsnprintf(message, sizeof(message), fmt, ap);
            traceMessages.push_back(message);
            traceWrites++;
        }
    }
};

class NoOpFrameFilter : public FrameFilter {
public:
    int executions;

    NoOpFrameFilter()
        : executions(0) { }

    virtual const char* name() const { return "test-filter"; }

    virtual void execute(FrameFilterFrame&) { executions++; }
};

class CopiedSourceProbeFilter : public FrameFilter {
public:
    int executions;

    CopiedSourceProbeFilter()
        : executions(0) { }

    virtual const char* name() const { return "copied-source-probe"; }
    virtual FrameFilterBufferContract bufferContract() const {
        return FrameFilterPixelCopiedSource;
    }
    virtual void execute(FrameFilterFrame& frame) {
        FrameStageBuffer& buffer = frame.buffer();
        assert(buffer.destinationPixels()[0] == 22);
        assert(buffer.sourcePixels()[0] == 22);
        buffer.destinationPixels()[0] = 33;
        executions++;
    }
};

class FreshDestinationProbeFilter : public FrameFilter {
public:
    int executions;

    FreshDestinationProbeFilter()
        : executions(0) { }

    virtual const char* name() const { return "fresh-destination-probe"; }
    virtual FrameFilterBufferContract bufferContract() const {
        return FrameFilterPixelFreshDestination;
    }
    virtual void execute(FrameFilterFrame& frame) {
        FrameStageBuffer& buffer = frame.buffer();
        assert(buffer.sourcePixels()[0] == 33);
        assert(buffer.destinationPixels()[0] == 22);
        buffer.destinationPixels()[0] = 44;
        executions++;
    }
};

class CommitProbeFilter : public FrameFilter {
public:
    int executions;

    CommitProbeFilter()
        : executions(0) { }

    virtual const char* name() const { return "commit-probe"; }
    virtual FrameFilterBufferContract bufferContract() const {
        return FrameFilterCommitBoundary;
    }
    virtual void execute(FrameFilterFrame& frame) {
        FrameStageBuffer& buffer = frame.buffer();
        assert(buffer.destinationPixels()[0] == 44);
        assert(buffer.sourcePixels()[0] == 33);
        executions++;
    }
};

class PublishProbeFilter : public FrameFilter {
public:
    int executions;

    PublishProbeFilter()
        : executions(0) { }

    virtual const char* name() const { return "publish-probe"; }
    virtual FrameFilterBufferContract bufferContract() const {
        return FrameFilterPublish;
    }
    virtual void execute(FrameFilterFrame& frame) {
        FrameStageBuffer& buffer = frame.buffer();
        assert(buffer.sourcePixels()[0] == 44);
        assert(buffer.destinationPixels()[0] == 33);
        executions++;
    }
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

    FrameRenderTarget target;
    FrameGeneratorContext context;
    assert(filterchain.stageEnabled(7) == 1);
    assert(filterchain.setStageEnabled(7, 0) == 1);
    assert(filterchain.stageEnabled(7) == 0);
    filterchain.run(target, context);
    assert(filter.executions == 0);
    assert(filterchain.setStageEnabled(7, 1) == 1);
    assert(filterchain.stageEnabled(7) == 1);
    filterchain.run(target, context);
    assert(filter.executions == 1);

    int sawFilterTiming = 0;
    int sawRunTiming = 0;
    for (std::vector<std::string>::const_iterator it = log.traceMessages.begin();
         it != log.traceMessages.end(); ++it) {
        if (it->find("filter-ms=") != std::string::npos
            && it->find("filter=test-filter") != std::string::npos)
            sawFilterTiming = 1;
        if (it->find("run-ms=") != std::string::npos)
            sawRunTiming = 1;
    }

    assert(sawFilterTiming);
    assert(sawRunTiming);
}

static void testFrameFilterchainCoordinatesBufferContracts() {
    RecordingLogSink log;
    FrameFilterchain filterchain(log);
    CopiedSourceProbeFilter destination;
    FreshDestinationProbeFilter transform;
    CommitProbeFilter commit;
    PublishProbeFilter publish;

    filterchain.add(1, &destination);
    filterchain.add(2, &transform);
    filterchain.add(3, &commit);
    filterchain.add(4, &publish);

    std::vector<unsigned int> stages;
    stages.push_back(1);
    stages.push_back(2);
    stages.push_back(3);
    stages.push_back(4);
    filterchain.setStageSequence(stages);
    filterchain.setStageMode(1, FrameFilterEnabled);
    filterchain.setStageMode(2, FrameFilterEnabled);
    filterchain.setStageMode(3, FrameFilterEnabled);
    filterchain.setStageMode(4, FrameFilterEnabled);

    FrameStore store;
    store.resize(FrameStorageLayout(PixelSize(2, 1), 2, 1));
    FrameRenderTarget& target = store.renderTarget();
    target.destinationPixels()[0] = 11;
    target.sourcePixels()[0] = 22;

    FrameGeneratorContext context;
    filterchain.run(target, context);

    assert(destination.executions == 1);
    assert(transform.executions == 1);
    assert(commit.executions == 1);
    assert(publish.executions == 1);
    assert(target.sourcePixels()[0] == 44);
    assert(target.destinationPixels()[0] == 33);
}

int main() {
    testFrameFilterchainUsesInjectedLogSink();
    testFrameFilterchainCoordinatesBufferContracts();
    return 0;
}
