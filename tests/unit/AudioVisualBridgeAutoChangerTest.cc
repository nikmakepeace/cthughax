/** @file
 * Unit coverage for AudioVisualBridge-owned AutoChanger policy.
 */

#include "AudioVisualBridge.h"
#include "AutoChangeSettings.h"
#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioProcessing.h"
#include "AudioProcessor.h"
#include "RuntimeCommandSink.h"
#include "VideoDirector.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
double getTime() { return 0.0; }
int audioBytesPerSample() { return 2; }

static int fakeTime = 1000;

int gettime() {
    return fakeTime;
}

VideoDirector& videoDirector() {
    static char storage[sizeof(VideoDirector)];
    return *reinterpret_cast<VideoDirector*>(storage);
}

int VideoDirector::observeQuiet(int) {
    return 0;
}

class RecordingSink : public RuntimeCommandSink {
public:
    std::vector<RuntimeCommandType> commands;

    virtual RuntimeChangeSet apply(const RuntimeCommand& command) {
        commands.push_back(command.type);
        return RuntimeChangeSet();
    }
};

static void fillConstant(AudioFrame& frame, int left, int right) {
    frame.samples = 1024;
    for (int i = 0; i < 1024; i++) {
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static AutoChangeConfig autoChangeConfigWithLittleChanges() {
    AutoChangeConfig config;
    config.quietMs = 0;
    config.waitMinMs = 0;
    config.waitRandomMs = 1;
    config.cumulativeFireLevel = 0;
    config.locked = 0;
    config.changeLittle = 1;
    return config;
}

static void testBridgeRoutesAutoChangerCommandsThroughSink() {
    fakeTime = 1000;

    OwnedAutoChangeSettings settings(autoChangeConfigWithLittleChanges());
    RecordingSink sink;
    AcousticContext acousticContext;
    AudioProcessor processor;
    AudioProcessingState processingState;
    AudioProcessingSelector processingSelector(processingState, processor);
    AudioVisualBridge bridge(acousticContext, processingSelector, processor,
        4, &sink, &settings);

    processingSelector.changeTo("none");
    AudioFrame frame;
    fillConstant(frame, 8, 8);

    fakeTime = 1002;
    bridge.runFrame(frame);

    assert(sink.commands.size() == 1);
    assert(sink.commands[0] == RuntimeCommandChangeOne);
    assert(strlen(bridge.autoChangerStatus()) > 0);
}

static void testBridgeWithoutSinkHasNoAutoChangerStatus() {
    AcousticContext acousticContext;
    AudioProcessor processor;
    AudioProcessingState processingState;
    AudioProcessingSelector processingSelector(processingState, processor);
    AudioVisualBridge bridge(acousticContext, processingSelector, processor, 4);

    assert(strcmp(bridge.autoChangerStatus(), "") == 0);
}

int main() {
    testBridgeRoutesAutoChangerCommandsThroughSink();
    testBridgeWithoutSinkHasNoAutoChangerStatus();
    return 0;
}
