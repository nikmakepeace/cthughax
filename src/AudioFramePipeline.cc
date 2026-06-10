/** @file
 * Default per-frame audio processing pipeline.
 */

#include "AudioFramePipeline.h"

#include "Audio.h"
#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioProcessing.h"
#include "ProcessServices.h"

DefaultAudioFramePipeline::DefaultAudioFramePipeline(
    AcousticContext& acousticContext_,
    AudioProcessingSelector& audioProcessingSelector_,
    AudioProcessor& audioProcessor_, SecondsClock& clock_, LogSink& log_,
    int minNoise_)
    : acousticContextValue(acousticContext_)
    , audioProcessingSelectorValue(audioProcessingSelector_)
    , audioProcessorValue(audioProcessor_)
    , clock(clock_)
    , log(log_)
    , minNoiseValue(minNoise_)
    , debugReportsValue(0) {
    log.debug("audio frame pipeline: creating pipeline\n");
}

void DefaultAudioFramePipeline::processFrame(AudioFrame& frame) {
    int traceFrame = log.traceEnabled();
    double start = traceFrame ? clock.nowSeconds() : 0.0;

    audioProcessingSelectorValue.process(frame);
    double processed = traceFrame ? clock.nowSeconds() : 0.0;

    if (log.debugEnabled() && (debugReportsValue < 16)) {
        AudioMetrics processedMetrics = audioProcessorValue.analyze(
            frame.processedWaveData, minNoiseValue);
        debugReportsValue++;
        log.debug("processed wave audio: mode=%s amplitude=%d left=%d right=%d lowpass150hz=%d noisy=%d\n",
            audioProcessingSelectorValue.text(), processedMetrics.amplitude,
            processedMetrics.amplitudeLeft, processedMetrics.amplitudeRight,
            processedMetrics.lowPass150HzAmplitude, processedMetrics.noisy);
    }

    audioProcessorValue.analyze(frame, minNoiseValue);
    acousticContextValue.update(frame.metrics);

    if (log.debugEnabled() && (debugReportsValue < 16)) {
        log.debug("audio analysis: amplitude=%d left=%d right=%d lowpass150hz=%d noisy=%d frame-samples=%d center-sample=%lld\n",
            frame.metrics.amplitude, frame.metrics.amplitudeLeft,
            frame.metrics.amplitudeRight, frame.metrics.lowPass150HzAmplitude,
            frame.metrics.noisy, frame.samples, frame.centerSample);
    }

    if (traceFrame) {
        double done = clock.nowSeconds();
        log.trace("audio frame pipeline",
            "frame-ms=%.3f process-ms=%.3f analyze-ms=%.3f\n",
            (done - start) * 1000.0,
            (processed - start) * 1000.0,
            (done - processed) * 1000.0);
    }
}

AcousticContext& DefaultAudioFramePipeline::acousticContext() {
    return acousticContextValue;
}

const AcousticContext& DefaultAudioFramePipeline::acousticContext() const {
    return acousticContextValue;
}

int DefaultAudioFramePipeline::debugReportCount() const {
    return debugReportsValue;
}
