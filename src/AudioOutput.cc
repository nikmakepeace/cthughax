/** @file
 * Audio output base class and null output backend.
 */

#include "Audio.h"
#include "AudioInternal.h"
#include "ProcessServices.h"

AudioOutput::AudioOutput(int targetLatencyMs, AudioOutputDump* outputDump,
    SecondsClock& clock_, LogSink& log_)
    : clock(clock_)
    , log(log_)
    , outputSamplesPerSecond(0)
    , outputBytesPerSample(1)
    , outputTargetLatencyMs(targetLatencyMs)
    , outputTargetDelaySamples(0)
    , outputScratchSamples(0)
    , outputDumpValue(outputDump) { }

AudioOutput::~AudioOutput() { }

double AudioOutput::nowSeconds() const {
    return clock.nowSeconds();
}

LogSink& AudioOutput::logSink() const {
    return log;
}

int AudioOutput::defaultTargetLatencyMs() const {
    return outputTargetLatencyMs;
}

int AudioOutput::timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const {
    return isRealtime() ? targetDelaySamples : inputChunkSamples;
}

void AudioOutput::dumpSubmittedPcm(const PcmFormat& format, const char* data,
    int bytes) {
    if (outputDumpValue != NULL)
        outputDumpValue->append(format, data, bytes);
}

void AudioOutput::reportSubmittedPcm(const PcmFormat& format,
    const char* scratch, int samples, int bytes, int written,
    int queuedSamples, long long submittedEndSample) {
    submittedPcmDebugReporterValue.submittedPcm(format, scratch, samples,
        bytes, written, queuedSamples, submittedEndSample, log);
}

void AudioOutput::configureTiming(int samplesPerSecond, int bytesPerSample, int inputChunkSamples) {
    outputSamplesPerSecond = samplesPerSecond;
    outputBytesPerSample = bytesPerSample;
    int targetLatencyMs = defaultTargetLatencyMs();
    outputScratchSamples = inputChunkSamples;

    if (outputBytesPerSample < 1)
        outputBytesPerSample = 1;
    if (outputScratchSamples < 1)
        outputScratchSamples = 1;
    if (targetLatencyMs < 0)
        targetLatencyMs = 0;

    outputTargetDelaySamples = (outputSamplesPerSecond * targetLatencyMs) / 1000;
    if (outputTargetDelaySamples < outputScratchSamples)
        outputTargetDelaySamples = outputScratchSamples;

    outputScratchSamples = timingScratchSamples(inputChunkSamples, outputTargetDelaySamples);

    if (outputScratchSamples < 1)
        outputScratchSamples = inputChunkSamples;
    if (outputTargetDelaySamples < 1)
        outputTargetDelaySamples = outputScratchSamples;

    log.debug("audio output: configured timing realtime=%d samples-per-second=%d bytes-per-sample=%d input-chunk-samples=%d target-buffer-ms=%d target-buffer-samples=%d scratch-samples=%d\n",
        isRealtime(), outputSamplesPerSecond, outputBytesPerSample,
        inputChunkSamples, targetLatencyMs, outputTargetDelaySamples, outputScratchSamples);
}

int AudioOutput::queuedTargetSamples() const {
    return isRealtime() ? outputTargetDelaySamples : outputScratchSamples;
}

int AudioOutput::playbackComplete(const AudioOutputStream& stream, int inputFinished) const {
    return inputFinished && (stream.queuedForOutputSamples() == 0);
}

AudioNullOutput::AudioNullOutput(SecondsClock& clock_, LogSink& log_,
    const AudioOutputConfig& config,
    AudioOutputDump* outputDump)
    : AudioOutput(config.nullOutputTargetLatencyMs, outputDump, clock_, log_) { }

int AudioNullOutput::defaultTargetLatencyMs() const {
    return AudioOutput::defaultTargetLatencyMs();
}

int AudioNullOutput::write(const void*, int size) { return size; }
int AudioNullOutput::isOpen() const { return 1; }
int AudioNullOutput::isRealtime() const { return 0; }
int AudioOutput::service(AudioOutputStream& stream, char* scratch, int scratchSamples,
    int /*inputFinished*/) {
    if ((scratch == NULL) || (scratchSamples <= 0))
        return 0;

    double serviceStart = nowSeconds();
    int bytesPerSample = stream.bytesPerSample();
    const PcmFormat& format = stream.format();
    if (bytesPerSample <= 0)
        return 0;

    int skippedSamples = stream.resyncIfBehind();
    if (skippedSamples > 0) {
        log.warn("Audio passthrough fell behind decoded history; skipped %d samples.\n",
            skippedSamples);
    }

    int queuedBefore = stream.queuedForOutputSamples();
    if (queuedBefore <= 0)
        return 0;

    int samplesWanted = queuedBefore;
    if (samplesWanted > queuedBefore)
        samplesWanted = queuedBefore;
    if (samplesWanted > scratchSamples)
        samplesWanted = scratchSamples;
    if (samplesWanted <= 0)
        return 0;

    log.trace("audio timing",
        "service plan realtime=%d queued-samples=%d scratch-samples=%d requested-samples=%d\n",
        isRealtime(), queuedBefore, scratchSamples, samplesWanted);

    double bufferReadStart = nowSeconds();
    long long startSample = stream.submittedEndPosition();
    int samples = stream.peekForOutput(scratch, samplesWanted);
    double bufferReadEnd = nowSeconds();
    if (samples <= 0)
        return 0;

    int bytes = pcmBytesForSamples(samples, bytesPerSample);
    double outputWriteStart = nowSeconds();
    int written = write(scratch, bytes);
    double outputWriteEnd = nowSeconds();
    int committedSamples = 0;
    int committedBytes = 0;
    if (written > 0) {
        committedSamples = stream.commitOutputSamples(written / bytesPerSample);
        committedBytes = pcmBytesForSamples(committedSamples, bytesPerSample);
    }
    if (committedSamples > 0) {
        dumpSubmittedPcm(format, scratch, committedBytes);
    }

    reportSubmittedPcm(format, scratch, committedSamples, committedBytes,
        written, stream.queuedForOutputSamples(), stream.submittedEndPosition());
    log.trace("audio runtime",
        "output submitted samples=%d bytes=%d written=%d committed-samples=%d committed-bytes=%d queued-samples=%d submitted-start-sample=%lld submitted-end-sample=%lld requested-samples=%d\n",
        samples, bytes, written, committedSamples, committedBytes,
        stream.queuedForOutputSamples(), startSample, stream.submittedEndPosition(),
        samplesWanted);
    log.trace("audio timing",
        "drain buffer-read-ms=%.3f output-write-ms=%.3f service-ms=%.3f samples=%d bytes=%d written=%d committed-samples=%d queued-samples=%d\n",
        (bufferReadEnd - bufferReadStart) * 1000.0,
        (outputWriteEnd - outputWriteStart) * 1000.0,
        (nowSeconds() - serviceStart) * 1000.0,
        samples, bytes, written, committedSamples,
        stream.queuedForOutputSamples());

    return committedSamples > 0 ? 1 : 0;
}
