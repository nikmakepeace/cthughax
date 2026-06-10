/** @file
 * Unit coverage for the default per-frame audio pipeline.
 */

#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioFramePipeline.h"
#include "AudioProcessing.h"
#include "AudioProcessor.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    int debugEnabledValue;

    FakeLogSink()
        : debugEnabledValue(0) { }

    virtual int enabled(int level) const {
        return debugEnabledValue && (level == CTH_LOG_DEBUG);
    }
};

class FakeRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) {
        return 0;
    }
};

class FakeSecondsClock : public SecondsClock {
public:
    double value;

    FakeSecondsClock()
        : value(0.0) { }

    virtual double nowSeconds() const {
        return value;
    }
};

static void fillConstant(AudioFrame& frame, int left, int right) {
    frame.samples = 1024;
    for (int i = 0; i < 1024; i++) {
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static void testPipelineProcessesAndAnalyzesSuppliedFrame() {
    AudioFrame frame;
    AudioProcessor processor;
    FakeRandomSource randomSource;
    FakeSecondsClock clock;
    FakeLogSink log;
    AcousticContext acousticContext(&log);
    AudioProcessingState processingState(randomSource);
    AudioProcessingSelector processingSelector(processingState, processor, log);
    DefaultAudioFramePipeline pipeline(acousticContext, processingSelector,
        processor, clock, log, 4);

    processingSelector.changeTo("none");
    fillConstant(frame, 3, 4);

    pipeline.processFrame(frame);

    assert(frame.metrics.amplitudeLeft == 3);
    assert(frame.metrics.amplitudeRight == 4);
    assert(frame.metrics.amplitude == 3);
    assert(frame.metrics.lowPass150HzAmplitudeLeft == 3);
    assert(frame.metrics.lowPass150HzAmplitudeRight == 4);
    assert(frame.metrics.lowPass150HzAmplitude == 3);
    assert(frame.metrics.noisy == 1);
    assert(frame.processedWaveData[0][0] == 3);
    assert(frame.processedWaveData[0][1] == 4);
    assert(acousticContext.intensity() > 0.0);
    assert(&pipeline.acousticContext() == &acousticContext);
}

static void testPipelineDebugReportsAreInstanceLocal() {
    AudioFrame firstFrame;
    AudioFrame secondFrame;
    AudioProcessor firstProcessor;
    AudioProcessor secondProcessor;
    FakeRandomSource firstRandomSource;
    FakeRandomSource secondRandomSource;
    FakeSecondsClock firstClock;
    FakeSecondsClock secondClock;
    FakeLogSink firstLog;
    FakeLogSink secondLog;
    AcousticContext firstAcousticContext(&firstLog);
    AcousticContext secondAcousticContext(&secondLog);
    AudioProcessingState firstProcessingState(firstRandomSource);
    AudioProcessingState secondProcessingState(secondRandomSource);
    AudioProcessingSelector firstProcessingSelector(firstProcessingState,
        firstProcessor, firstLog);
    AudioProcessingSelector secondProcessingSelector(secondProcessingState,
        secondProcessor, secondLog);
    DefaultAudioFramePipeline first(firstAcousticContext, firstProcessingSelector,
        firstProcessor, firstClock, firstLog, 4);
    DefaultAudioFramePipeline second(secondAcousticContext, secondProcessingSelector,
        secondProcessor, secondClock, secondLog, 4);

    fillConstant(firstFrame, 2, 3);
    fillConstant(secondFrame, 4, 5);
    firstLog.debugEnabledValue = 1;
    secondLog.debugEnabledValue = 1;

    first.processFrame(firstFrame);
    assert(first.debugReportCount() == 1);
    assert(second.debugReportCount() == 0);

    second.processFrame(secondFrame);
    assert(first.debugReportCount() == 1);
    assert(second.debugReportCount() == 1);

    for (int i = 0; i < 24; i++)
        first.processFrame(firstFrame);
    assert(first.debugReportCount() == 16);
    assert(second.debugReportCount() == 1);
}

int main() {
    testPipelineProcessesAndAnalyzesSuppliedFrame();
    testPipelineDebugReportsAreInstanceLocal();
    return 0;
}
