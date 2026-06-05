// Audio output base class, null output, and output option globals.

#include "cthugha.h"
#include "Audio.h"
#include "AudioInternal.h"
#include "Configuration.h"

#include <string.h>

char pulse_server[PATH_MAX] = "";
int pulse_latency_msec = 0;
char audio_output_dump[PATH_MAX] = "";
int audio_null_target_latency_msec = 0;
int audio_pulse_target_latency_msec = 0;
int audio_dsp_target_latency_msec = 0;

void configureAudioOutputOptions(const AudioConfig& config) {
    strncpy(pulse_server, config.pulseServer.c_str(), PATH_MAX);
    pulse_server[PATH_MAX - 1] = '\0';
    pulse_latency_msec = config.pulseLatencyMs;
    strncpy(audio_output_dump, config.outputDumpPath.c_str(), PATH_MAX);
    audio_output_dump[PATH_MAX - 1] = '\0';
    audio_null_target_latency_msec = config.nullOutputTargetLatencyMs;
    audio_pulse_target_latency_msec = config.pulseOutputTargetLatencyMs;
    audio_dsp_target_latency_msec = config.dspOutputTargetLatencyMs;
}

const char* pulse_server_name() {
    return (pulse_server[0] != '\0') ? pulse_server : NULL;
}

const char* pulse_server_display_name() {
    return pulse_server_name() ? pulse_server_name() : "default";
}

AudioOutput::AudioOutput()
    : outputSamplesPerSecond(0)
    , outputBytesPerSample(1)
    , outputTargetDelaySamples(0)
    , outputScratchSamples(0) { }

AudioOutput::~AudioOutput() { }

int AudioOutput::timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const {
    return isRealtime() ? targetDelaySamples : inputChunkSamples;
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

    CTH_DEBUG("audio output: configured timing realtime=%d samples-per-second=%d bytes-per-sample=%d input-chunk-samples=%d target-buffer-ms=%d target-buffer-samples=%d scratch-samples=%d\n",
        isRealtime(), outputSamplesPerSecond, outputBytesPerSample,
        inputChunkSamples, targetLatencyMs, outputTargetDelaySamples, outputScratchSamples);
}

int AudioOutput::queuedTargetSamples() const {
    return isRealtime() ? outputTargetDelaySamples : outputScratchSamples;
}

int AudioOutput::playbackComplete(const AudioOutputStream& stream, int inputFinished) const {
    return inputFinished && (stream.queuedForOutputSamples() == 0);
}

int AudioNullOutput::defaultTargetLatencyMs() const { return audio_null_target_latency_msec; }
int AudioNullOutput::write(const void*, int size) { return size; }
int AudioNullOutput::isOpen() const { return 1; }
int AudioNullOutput::isRealtime() const { return 0; }
int AudioOutput::service(AudioOutputStream& stream, char* scratch, int scratchSamples,
    int /*inputFinished*/) {
    if ((scratch == NULL) || (scratchSamples <= 0))
        return 0;

    double serviceStart = getTime();
    int bytesPerSample = stream.bytesPerSample();
    if (bytesPerSample <= 0)
        return 0;

    int skippedSamples = stream.resyncIfBehind();
    if (skippedSamples > 0) {
        CTH_WARN("Audio passthrough fell behind decoded history; skipped %d samples.\n",
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

    CTH_TRACE("service plan realtime=%d queued-samples=%d scratch-samples=%d requested-samples=%d\n",
        "audio timing", isRealtime(), queuedBefore, scratchSamples, samplesWanted);

    double bufferReadStart = getTime();
    long long startSample = stream.submittedEndPosition();
    int samples = stream.peekForOutput(scratch, samplesWanted);
    double bufferReadEnd = getTime();
    if (samples <= 0)
        return 0;

    int bytes = pcmBytesForSamples(samples, bytesPerSample);
    double outputWriteStart = getTime();
    int written = write(scratch, bytes);
    double outputWriteEnd = getTime();
    int committedSamples = 0;
    int committedBytes = 0;
    if (written > 0) {
        committedSamples = stream.commitOutputSamples(written / bytesPerSample);
        committedBytes = pcmBytesForSamples(committedSamples, bytesPerSample);
    }
    if (committedSamples > 0) {
        audioOutputDumpSubmittedPcm(scratch, committedBytes);
    }

    audioDebugSubmittedPcm(scratch, committedSamples, committedBytes, written, stream.queuedForOutputSamples(),
        stream.submittedEndPosition());
    CTH_TRACE("output submitted samples=%d bytes=%d written=%d committed-samples=%d committed-bytes=%d queued-samples=%d submitted-start-sample=%lld submitted-end-sample=%lld requested-samples=%d\n", "audio runtime",
        samples, bytes, written, committedSamples, committedBytes,
        stream.queuedForOutputSamples(), startSample, stream.submittedEndPosition(),
        samplesWanted);
    CTH_TRACE("drain buffer-read-ms=%.3f output-write-ms=%.3f service-ms=%.3f samples=%d bytes=%d written=%d committed-samples=%d queued-samples=%d\n", "audio timing",
        (bufferReadEnd - bufferReadStart) * 1000.0,
        (outputWriteEnd - outputWriteStart) * 1000.0,
        (getTime() - serviceStart) * 1000.0,
        samples, bytes, written, committedSamples,
        stream.queuedForOutputSamples());

    return committedSamples > 0 ? 1 : 0;
}
