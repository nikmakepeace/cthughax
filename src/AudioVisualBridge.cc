/** @file
 * Audio frame processing, analysis, and audio-driven scene policy.
 */

#include "cthugha.h"
#include "AudioVisualBridge.h"
#include "Audio.h"
#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "AudioFrame.h"
#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
#include "AutoChanger.h"
#endif

AudioVisualBridge::AudioVisualBridge(AcousticContext& acousticContext_,
    int minNoise_, RuntimeCommandSink* runtimeCommands_)
    : filterchainRefreshRequestedValue(0)
    , acousticContextValue(acousticContext_)
    , minNoiseValue(minNoise_) {
    CTH_DEBUG("audio visual bridge: creating bridge\n");
#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
    if (runtimeCommands_ != 0)
        autoChangerValue.reset(new AutoChanger(*runtimeCommands_, acousticContextValue));
#endif
}

AudioVisualBridge::~AudioVisualBridge() {
    CTH_DEBUG("audio visual bridge: destroying bridge\n");
}

void AudioVisualBridge::runFrame(AudioFrame& frame) {
    double start = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;
    static int debugReports = 0;
    static AudioProcessor processor;

    audioProcessing.process(frame);
    double processed = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        AudioMetrics processedMetrics = processor.analyze(
            frame.processedWaveData, minNoiseValue);
        debugReports++;
        CTH_DEBUG("processed wave audio: mode=%s amplitude=%d left=%d right=%d noisy=%d\n",
            audioProcessing.text(), processedMetrics.amplitude,
            processedMetrics.amplitudeLeft, processedMetrics.amplitudeRight,
            processedMetrics.noisy);
    }

    processor.analyze(frame, minNoiseValue);
    acousticContextValue.update(frame.metrics);

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        CTH_DEBUG("audio analysis: amplitude=%d left=%d right=%d noisy=%d frame-samples=%d center-sample=%lld\n",
            frame.metrics.amplitude, frame.metrics.amplitudeLeft,
            frame.metrics.amplitudeRight, frame.metrics.noisy,
            frame.samples, frame.centerSample);
    }

    double analyzed = CTH_LOG_ENABLED(CTH_LOG_TRACE) ? getTime() : 0.0;

#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
    if (autoChangerValue.get() != 0)
        (*autoChangerValue)(frame.metrics);
#endif

    if (CTH_LOG_ENABLED(CTH_LOG_TRACE)) {
        double done = getTime();
        CTH_TRACE("frame-ms=%.3f process-ms=%.3f analyze-ms=%.3f auto-ms=%.3f refresh-requested=%d\n",
            "audio visual bridge",
            (done - start) * 1000.0,
            (processed - start) * 1000.0,
            (analyzed - processed) * 1000.0,
            (done - analyzed) * 1000.0,
            filterchainRefreshRequestedValue);
    }
}

const char* AudioVisualBridge::autoChangerStatus() const {
#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
    if (autoChangerValue.get() != 0)
        return autoChangerValue->status();
#endif
    return "";
}
