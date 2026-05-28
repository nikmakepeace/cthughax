#include "cthugha.h"
#include "AudioVisualBridge.h"
#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "AudioFrame.h"
#include "AutoChanger.h"

AudioVisualBridge::AudioVisualBridge()
    : pipelineRefreshRequestedValue(0) {
    CTH_TRACE("creating bridge\n", "audio visual bridge");
    autoChanger = new AutoChanger;
}

AudioVisualBridge::~AudioVisualBridge() {
    CTH_TRACE("destroying bridge\n", "audio visual bridge");
    delete autoChanger;
    autoChanger = 0;
}

void AudioVisualBridge::runFrame() {
    double start = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;
    static int debugReports = 0;

    audioProcessing.process();
    double processed = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        AudioAnalysis processedAnalysis = audioAnalyzer.analyze(audioFrameProcessedData());
        debugReports++;
        CTH_DEBUG("processed audio: mode=%s amplitude=%d left=%d right=%d noisy=%d\n",
            audioProcessing.text(), processedAnalysis.amplitude,
            processedAnalysis.amplitudeLeft, processedAnalysis.amplitudeRight,
            processedAnalysis.noisy);
    }

    audioAnalyzer();
    double analyzed = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;

    if (autoChanger != 0)
        (*autoChanger)();

    if (CTH_LOG_ENABLED(CTH_LOG_TRACE)) {
        double done = getTime();
        CTH_TRACE("frame-ms=%.3f process-ms=%.3f analyze-ms=%.3f auto-ms=%.3f refresh-requested=%d\n",
            "audio visual bridge",
            (done - start) * 1000.0,
            (processed - start) * 1000.0,
            (analyzed - processed) * 1000.0,
            (done - analyzed) * 1000.0,
            pipelineRefreshRequestedValue);
    }
}
