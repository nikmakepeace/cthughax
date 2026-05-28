#include "cthugha.h"
#include "AudioRuntime.h"
#include "CthughaDisplay.h"

#include <atomic>
#include <chrono>
#include <thread>

static AudioInputProcessor* audioProcessor = NULL;
static AudioInput* audioInput = NULL;
static AudioOutput* audioOutput = NULL;
static AudioBuffer* audioBuffer = NULL;
static AudioFrameBuilder* audioFrameBuilder = NULL;
static char* audioRuntimeChunk = NULL;
static char* audioRuntimeOutputChunk = NULL;
static AudioFrame audioRuntimeFrame;
static int audioRuntimeChunkSamples = 0;
static int audioRuntimeOutputChunkSamples = 0;
static std::atomic<int> audioRuntimeInputFinished(0);
static std::atomic<int> audioRuntimeComplete(0);
static std::atomic<int> audioRuntimeThreadsStop(0);
static std::thread audioRuntimeInputThread;
static std::thread audioRuntimeOutputThread;
static int audioRuntimeThreadsStarted = 0;
static std::atomic<int> audioRuntimeCallbackDrainStarted(0);
static int audioRuntimeCompletionAnnounced = 0;

static long long audioRuntimeDecodedSamplePosition();
static long long audioRuntimeSubmittedSamplePosition();
static long long audioRuntimeAudibleSamplePosition();
static void audioRuntimeAnnounceComplete();

static int audioRuntimeReadSigned16Le(const unsigned char* p) {
    unsigned int value = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
    return (value & 0x8000) ? (int)value - 0x10000 : (int)value;
}

static int audioRuntimeReadSigned16Be(const unsigned char* p) {
    unsigned int value = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    return (value & 0x8000) ? (int)value - 0x10000 : (int)value;
}

static unsigned int audioRuntimeReadUnsigned16Le(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int audioRuntimeReadUnsigned16Be(const unsigned char* p) {
    return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}

static void audioRuntimeUpdatePeak(int& peak, int sample) {
    sample = abs(sample);
    if (sample > peak)
        peak = sample;
}

static int audioRuntimePcmPeak(const char* data, int samples) {
    const unsigned char* bytes = (const unsigned char*)data;
    int peak = 0;
    int channels = int(soundChannels);
    if ((data == NULL) || (samples <= 0) || (channels <= 0))
        return 0;

    switch (soundFormat) {
    case SF_u8:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, (int)bytes[i] - 128);
        break;
    case SF_s8:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, (int)((const signed char*)data)[i]);
        break;
    case SF_u16_le:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, (int)audioRuntimeReadUnsigned16Le(bytes + i * 2) - 32768);
        break;
    case SF_s16_le:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, audioRuntimeReadSigned16Le(bytes + i * 2));
        break;
    case SF_u16_be:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, (int)audioRuntimeReadUnsigned16Be(bytes + i * 2) - 32768);
        break;
    case SF_s16_be:
        for (int i = 0; i < samples * channels; i++)
            audioRuntimeUpdatePeak(peak, audioRuntimeReadSigned16Be(bytes + i * 2));
        break;
    default:
        break;
    }

    return peak;
}

static void audioRuntimeDebugDecodedPcm(const char* data, int samples, int bytes,
    int appendedSamples, int queuedSamples) {
    static int reports = 0;

    if (!CTH_LOG_ENABLED(CTH_LOG_DEBUG) || (reports >= 8))
        return;

    reports++;
    CTH_DEBUG("    audio input: decoded samples=%d bytes=%d appended-samples=%d peak=%d queued-samples=%d decoded-end-sample=%lld format=%s channels=%d rate=%d\n",
        samples, bytes, appendedSamples, audioRuntimePcmPeak(data, samples), queuedSamples,
        audioRuntimeDecodedSamplePosition(), soundFormat.text(), int(soundChannels),
        int(soundSampleRate));
}

#ifndef WITH_MINIMP3
#define WITH_MINIMP3 1
#endif

static const char* audioRuntimeContextName(RuntimeSoundInputContext context) {
    return (context == RSIC_FileChild) ? "file child" : "main process";
}

static int audioRuntimeUsesNativeFilePipeline(AudioSourceStrategy sourceStrategy) {
    if (sourceStrategy == ASS_WavFile)
        return 1;
#if WITH_MINIMP3 == 1
    if (sourceStrategy == ASS_Mp3File)
        return 1;
#endif
    return 0;
}

static int audioRuntimeBytesPerSample() {
    return (soundFormat < 2) ? int(soundChannels) : 2 * int(soundChannels);
}

static int audioRuntimeSamplesPerSecond() {
    return int(soundSampleRate);
}

static int audioRuntimeStrategyChunkSamples() {
    int samples = audioRuntimeSamplesPerSecond() / 20;

    if (samples < 1024)
        samples = 1024;
    if (samples > 8192)
        samples = 8192;

    return samples;
}

static int audioRuntimeStrategyVisualSlackSamples() {
    int samplesPerSecond = audioRuntimeSamplesPerSecond();
    int visualSlackSamples = samplesPerSecond / 4;
    int observedVisualSlackSamples = 0;

    if (cthughaDisplay != NULL)
        observedVisualSlackSamples = (int)(cthughaDisplay->visualLatencySeconds()
            * samplesPerSecond);
    if (observedVisualSlackSamples > visualSlackSamples)
        visualSlackSamples = observedVisualSlackSamples;

    return visualSlackSamples + 1024;
}

static int audioRuntimeStrategyHistorySamples(const AudioOutput& output) {
    int samples = output.targetDelaySamples() + audioRuntimeStrategyVisualSlackSamples();
    int minimumSamples = audioRuntimeSamplesPerSecond();

    if (minimumSamples < 16384)
        minimumSamples = 16384;
    if (samples < minimumSamples)
        samples = minimumSamples;

    return samples;
}

static int audioRuntimeStrategyBufferSamples(const AudioOutput& output,
    int protectedHistorySamples, int inputChunkSamples) {
    int decodeAheadSamples = output.targetDelaySamples() + inputChunkSamples * 2;
    int samples = protectedHistorySamples + decodeAheadSamples;
    int minimumSamples = audioRuntimeSamplesPerSecond() * 3;

    if (minimumSamples < 32768)
        minimumSamples = 32768;
    if (samples < minimumSamples)
        samples = minimumSamples;

    return samples;
}

static int audioRuntimeFillBuffer(int maxSamples) {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL))
        return 0;
    if (audioRuntimeInputFinished.load())
        return 0;

    int bytesPerSample = audioRuntimeBytesPerSample();
    if (bytesPerSample <= 0)
        return 0;

    int freeSamples = audioBuffer->writableSamples();
    int limitedSamples = (maxSamples < freeSamples) ? maxSamples : freeSamples;
    int samplesWanted = (audioRuntimeChunkSamples < limitedSamples) ? audioRuntimeChunkSamples : limitedSamples;
    if (samplesWanted <= 0)
        return 0;

    double readStart = getTime();
    int samplesRead = audioInput->read(audioRuntimeChunk,
        pcmBytesForSamples(samplesWanted, bytesPerSample), samplesWanted);
    double readEnd = getTime();
    if (samplesRead > 0) {
        double bufferStart = getTime();
        int buffered = audioBuffer->appendDecodedPcm(audioRuntimeChunk, samplesRead);
        double bufferEnd = getTime();
        int bytesRead = pcmBytesForSamples(samplesRead, bytesPerSample);
        CTH_TRACE("input produced samples=%d bytes=%d appended-samples=%d queued-samples=%d decoded-end-sample=%lld\n", "audio runtime",
            samplesRead, bytesRead, buffered, audioBuffer->queuedForOutputSamples(),
            audioBuffer->decodedEndPosition());
        audioRuntimeDebugDecodedPcm(audioRuntimeChunk, samplesRead, bytesRead, buffered,
            audioBuffer->queuedForOutputSamples());
        CTH_TRACE("fill read-ms=%.3f buffer-write-ms=%.3f samples=%d bytes=%d queued-samples=%d\n", "audio timing",
            (readEnd - readStart) * 1000.0, (bufferEnd - bufferStart) * 1000.0,
            samplesRead, bytesRead, audioBuffer->queuedForOutputSamples());
        if ((buffered > 0) && audioRuntimeCallbackDrainStarted.load()
            && (audioOutput != NULL))
            audioOutput->notifyCallbackDrain();
        return buffered;
    }

    if (audioInput->isFinished()) {
        audioRuntimeInputFinished.store(1);
        if (audioRuntimeCallbackDrainStarted.load() && (audioOutput != NULL))
            audioOutput->notifyCallbackDrain();
        CTH_TRACE("input finished decoded-end-sample=%lld queued-samples=%d\n", "audio runtime",
            audioBuffer->decodedEndPosition(), audioBuffer->queuedForOutputSamples());
    }

    return 0;
}

static void audioRuntimeInputThreadMain() {
    CTH_TRACE("input thread started chunk-samples=%d\n", "audio runtime",
        audioRuntimeChunkSamples);

    while (!audioRuntimeThreadsStop.load() && !audioRuntimeInputFinished.load()) {
        int filled = audioRuntimeFillBuffer(audioRuntimeChunkSamples);
        if (filled <= 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    CTH_TRACE("input thread stopped stop=%d input-finished=%d decoded-end-sample=%lld queued-samples=%d\n",
        "audio runtime", audioRuntimeThreadsStop.load(), audioRuntimeInputFinished.load(),
        audioRuntimeDecodedSamplePosition(),
        audioBuffer ? audioBuffer->queuedForOutputSamples() : 0);
}

static void audioRuntimeOutputThreadMain() {
    int primed = 0;

    CTH_TRACE("output thread started output-chunk-samples=%d target-delay-samples=%d\n",
        "audio runtime", audioRuntimeOutputChunkSamples,
        audioOutput ? audioOutput->targetDelaySamples() : 0);

    while (!audioRuntimeThreadsStop.load() && !audioRuntimeComplete.load()) {
        if ((audioOutput == NULL) || (audioBuffer == NULL)
            || (audioRuntimeOutputChunk == NULL)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        if (!primed) {
            int queuedSamples = audioBuffer->queuedForOutputSamples();
            int primeSamples = audioOutput->targetDelaySamples();
            if (!audioRuntimeInputFinished.load() && (queuedSamples < primeSamples)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            primed = 1;
            CTH_TRACE("output thread primed queued-samples=%d prime-samples=%d input-finished=%d\n",
                "audio runtime", queuedSamples, primeSamples,
                audioRuntimeInputFinished.load());
        }

        int writeCalls = audioOutput->service(*audioBuffer, audioRuntimeOutputChunk,
            audioRuntimeOutputChunkSamples, audioRuntimeInputFinished.load());

        if (audioOutput->playbackComplete(*audioBuffer, audioRuntimeInputFinished.load())) {
            audioRuntimeComplete.store(1);
            break;
        }

        if (writeCalls <= 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CTH_TRACE("output thread stopped stop=%d complete=%d queued-samples=%d delay-samples=%d submitted-end-sample=%lld audible-sample=%lld\n",
        "audio runtime", audioRuntimeThreadsStop.load(), audioRuntimeComplete.load(),
        audioBuffer ? audioBuffer->queuedForOutputSamples() : 0,
        audioOutput ? audioOutput->outputDelaySamples() : 0,
        audioRuntimeSubmittedSamplePosition(), audioRuntimeAudibleSamplePosition());
}

static void audioRuntimeStartThreads() {
    audioRuntimeThreadsStop.store(0);
    if ((audioOutput != NULL) && (audioBuffer != NULL)
        && audioOutput->supportsCallbackDrain()) {
        audioRuntimeCallbackDrainStarted.store(1);
        audioOutput->startCallbackDrain(*audioBuffer, &audioRuntimeInputFinished,
            audioRuntimeOutputChunkSamples);
        audioRuntimeInputThread = std::thread(audioRuntimeInputThreadMain);
        CTH_TRACE("using output callback drain queued-samples=%d target-delay-samples=%d output-chunk-samples=%d\n",
            "audio runtime", audioBuffer->queuedForOutputSamples(),
            audioOutput->targetDelaySamples(), audioRuntimeOutputChunkSamples);
    } else {
        audioRuntimeCallbackDrainStarted.store(0);
        audioRuntimeInputThread = std::thread(audioRuntimeInputThreadMain);
        audioRuntimeOutputThread = std::thread(audioRuntimeOutputThreadMain);
    }
    audioRuntimeThreadsStarted = 1;
}

static void audioRuntimeStopThreads() {
    audioRuntimeThreadsStop.store(1);

    if ((audioOutput != NULL) && audioRuntimeCallbackDrainStarted.load())
        audioOutput->stopCallbackDrain();

    if (audioRuntimeInputThread.joinable())
        audioRuntimeInputThread.join();
    if (audioRuntimeOutputThread.joinable())
        audioRuntimeOutputThread.join();

    audioRuntimeCallbackDrainStarted.store(0);
    audioRuntimeThreadsStarted = 0;
}

static void audioRuntimePumpPipeline() {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL) || (audioRuntimeOutputChunk == NULL))
        return;

    if (audioRuntimeComplete.load())
        return;

    double pumpStart = getTime();
    int fillCalls = 0;
    int targetDelay = audioOutput->targetDelaySamples();

    int inputTarget = audioOutput->queuedTargetSamples();
    int loops = 0;
    while (!audioRuntimeInputFinished.load() && (audioBuffer->queuedForOutputSamples() < inputTarget)
        && (loops < 16)) {
        if (audioRuntimeFillBuffer(audioRuntimeChunkSamples) <= 0)
            break;
        fillCalls++;
        loops++;
    }

    int writeCalls = audioOutput->service(*audioBuffer, audioRuntimeOutputChunk,
        audioRuntimeOutputChunkSamples, audioRuntimeInputFinished.load());

    if (audioOutput->playbackComplete(*audioBuffer, audioRuntimeInputFinished.load()))
        audioRuntimeComplete.store(1);

    CTH_TRACE("pump total-ms=%.3f fills=%d writes=%d queued-samples=%d delay-samples=%d target-delay-samples=%d complete=%d\n", "audio timing",
        (getTime() - pumpStart) * 1000.0, fillCalls, writeCalls,
        audioBuffer->queuedForOutputSamples(), audioOutput->outputDelaySamples(), targetDelay,
        audioRuntimeComplete.load());

    audioRuntimeAnnounceComplete();
}

static void audioRuntimeBuildFrame() {
    if ((audioFrameBuilder == NULL) || (audioBuffer == NULL))
        return;

    long long audibleSample = audioRuntimeAudibleSamplePosition();
    double visualLatencySeconds = cthughaDisplay ? cthughaDisplay->visualLatencySeconds() : 0;
    long long visualOffsetSamples = (long long)(audioRuntimeSamplesPerSecond() * visualLatencySeconds);

    long long frameCenterSample = audibleSample + visualOffsetSamples;
    if (frameCenterSample > audioBuffer->decodedEndPosition())
        frameCenterSample = audioBuffer->decodedEndPosition();

    double buildStart = getTime();
    audioFrameBuilder->build(audioRuntimeFrame, *audioBuffer, frameCenterSample);
    CTH_TRACE("frame-build-ms=%.3f audible-sample=%lld visual-offset-samples=%lld visual-latency-ms=%.3f center-sample=%lld queued-samples=%d\n",
        "audio timing",
        (getTime() - buildStart) * 1000.0, audibleSample, visualOffsetSamples,
        visualLatencySeconds * 1000.0, frameCenterSample, audioBuffer->queuedForOutputSamples());
}

static void audioRuntimeAnnounceComplete() {
    if (!audioRuntimeComplete.load() || audioRuntimeCompletionAnnounced)
        return;

    audioRuntimeCompletionAnnounced = 1;
    CTH_INFO("Stopping...\n");
    CTH_TRACE("playback complete decoded-end-sample=%lld submitted-end-sample=%lld audible-sample=%lld\n",
        "audio runtime", audioRuntimeDecodedSamplePosition(),
        audioRuntimeSubmittedSamplePosition(), audioRuntimeAudibleSamplePosition());
    cthugha_close++;
}

void audioRuntimeInit(RuntimeSoundInputContext context, int initializeInputControls) {
    CTH_TRACE("init requested context=%s initialize-input-controls=%d\n", "audio runtime",
        audioRuntimeContextName(context), initializeInputControls);

    if (audioRuntimeIsInitialized()) {
        CTH_TRACE("init skipped because audio is already installed\n", "audio runtime");
        return;
    }

    Settings settings = Settings::fromCurrentOptions();
    Environment environment = Environment::detect();
    RuntimeFactory runtimeFactory(settings, environment);
    AudioSourceStrategy sourceStrategy = runtimeFactory.selectAudioSourceStrategy();
    audioRuntimeInputFinished.store(0);
    audioRuntimeComplete.store(0);
    audioRuntimeThreadsStop.store(0);
    audioRuntimeCallbackDrainStarted.store(0);
    audioRuntimeCompletionAnnounced = 0;

    if (audioRuntimeUsesNativeFilePipeline(sourceStrategy)) {
        audioInput = runtimeFactory.createAudioInput();
        if ((audioInput == NULL) || audioInput->hasError()) {
            CTH_TRACE("native file input construction failed strategy=%d\n", "audio runtime",
                sourceStrategy);
            delete audioInput;
            audioInput = NULL;
            exit(0);
        }

        audioOutput = runtimeFactory.createAudioOutput();
        if ((audioOutput == NULL) || !audioOutput->isOpen()) {
            CTH_TRACE("native file output construction failed strategy=%d\n", "audio runtime",
                sourceStrategy);
            delete audioInput;
            audioInput = NULL;
            delete audioOutput;
            audioOutput = NULL;
            exit(0);
        }

        int bytesPerSample = audioRuntimeBytesPerSample();
        audioRuntimeChunkSamples = audioRuntimeStrategyChunkSamples();
        audioOutput->configureTiming(audioRuntimeSamplesPerSecond(), bytesPerSample,
            audioRuntimeChunkSamples);
        audioRuntimeOutputChunkSamples = audioOutput->scratchSamples();
        int protectedHistorySamples = audioRuntimeStrategyHistorySamples(*audioOutput);
        int bufferSamples = audioRuntimeStrategyBufferSamples(*audioOutput,
            protectedHistorySamples, audioRuntimeChunkSamples);
        audioBuffer = new AudioBuffer(bufferSamples, bytesPerSample,
            protectedHistorySamples);
        audioFrameBuilder = new AudioFrameBuilder();
        audioRuntimeChunk = new char[pcmBytesForSamples(audioRuntimeChunkSamples, bytesPerSample)];
        audioRuntimeOutputChunk = new char[pcmBytesForSamples(audioRuntimeOutputChunkSamples, bytesPerSample)];
        audioRuntimeFrame.clear();
        audioRuntimeStartThreads();
        CTH_DEBUG("    audio runtime: native file pipeline rate=%d channels=%d format=%s bytes-per-sample=%d input-chunk-samples=%d output-chunk-samples=%d target-delay-samples=%d protected-history-samples=%d buffer-samples=%d\n",
            int(soundSampleRate), int(soundChannels), soundFormat.text(), bytesPerSample,
            audioRuntimeChunkSamples, audioRuntimeOutputChunkSamples,
            audioOutput->targetDelaySamples(), protectedHistorySamples, bufferSamples);
        CTH_TRACE("installed native file pipeline strategy=%d input=%p buffer=%p output=%p frame-builder=%p input-chunk-samples=%d output-chunk-samples=%d bytes-per-sample=%d target-delay-samples=%d protected-history-samples=%d protected-history-ms=%d buffer-samples=%d buffer-ms=%d visual-slack-samples=%d\n", "audio runtime",
            sourceStrategy, audioInput, audioBuffer, audioOutput, audioFrameBuilder,
            audioRuntimeChunkSamples, audioRuntimeOutputChunkSamples, bytesPerSample,
            audioOutput->targetDelaySamples(), protectedHistorySamples,
            (protectedHistorySamples * 1000) / audioRuntimeSamplesPerSecond(),
            bufferSamples, (bufferSamples * 1000) / audioRuntimeSamplesPerSecond(),
            audioRuntimeStrategyVisualSlackSamples());
    } else {
        audioProcessor = runtimeFactory.createAudioProcessor();
        if (audioProcessor != NULL) {
            CTH_TRACE("installed native AudioInputProcessor path\n", "audio runtime");
            if (initializeInputControls && audioProcessor->audioInput()->initInputControls()) {
                CTH_TRACE("native input control initialization failed\n", "audio runtime");
                exit(0);
            }
        } else {
            CTH_TRACE("native AudioInputProcessor unavailable; using legacy SoundDevice path\n", "audio runtime");
            SoundDevice::install(runtimeFactory.createLegacySoundDevice(context), initializeInputControls);
        }
    }

    CTH_TRACE("init completed context=%s\n", "audio runtime", audioRuntimeContextName(context));
}

void audioRuntimeTick() {
    if (audioBuffer != NULL) {
        double tickStart = getTime();
        double pumpStart = tickStart;
        if (!audioRuntimeThreadsStarted)
            audioRuntimePumpPipeline();
        double pumpEnd = getTime();
        audioRuntimeBuildFrame();
        double buildEnd = getTime();
        if (audioRuntimeCallbackDrainStarted.load() && (audioOutput != NULL)
            && audioOutput->playbackComplete(*audioBuffer,
                audioRuntimeInputFinished.load()))
            audioRuntimeComplete.store(1);
        audioRuntimeAnnounceComplete();
        CTH_TRACE("tick pipeline-ms=%.3f pump-ms=%.3f build-ms=%.3f threaded=%d callback-drain=%d queued-samples=%d delay-samples=%d decoded-end-sample=%lld submitted-end-sample=%lld audible-sample=%lld complete=%d\n",
            "audio timing",
            (buildEnd - tickStart) * 1000.0,
            (pumpEnd - pumpStart) * 1000.0,
            (buildEnd - pumpEnd) * 1000.0,
            audioRuntimeThreadsStarted,
            audioRuntimeCallbackDrainStarted.load(),
            audioBuffer->queuedForOutputSamples(),
            audioOutput ? audioOutput->outputDelaySamples() : 0,
            audioRuntimeDecodedSamplePosition(),
            audioRuntimeSubmittedSamplePosition(),
            audioRuntimeAudibleSamplePosition(),
            audioRuntimeComplete.load());
        return;
    }

    if (audioProcessor != NULL) {
        (*audioProcessor)();
        return;
    }

    if (soundDevice)
        (*soundDevice)();
}

void audioRuntimeShutdown() {
    CTH_TRACE("shutdown requested pipeline=%d native=%d legacy=%d\n", "audio runtime",
        audioBuffer != NULL, audioProcessor != NULL, soundDevice != NULL);
    audioRuntimeStopThreads();

    delete[] audioRuntimeChunk;
    audioRuntimeChunk = NULL;
    delete[] audioRuntimeOutputChunk;
    audioRuntimeOutputChunk = NULL;

    delete audioBuffer;
    audioBuffer = NULL;

    delete audioFrameBuilder;
    audioFrameBuilder = NULL;
    audioRuntimeFrame.clear();

    delete audioOutput;
    audioOutput = NULL;

    delete audioInput;
    audioInput = NULL;

    delete audioProcessor;
    audioProcessor = NULL;

    audioRuntimeInputFinished.store(0);
    audioRuntimeComplete.store(0);
    audioRuntimeThreadsStop.store(0);
    audioRuntimeCompletionAnnounced = 0;
    audioRuntimeCallbackDrainStarted.store(0);
    audioRuntimeChunkSamples = 0;
    audioRuntimeOutputChunkSamples = 0;

    delete soundDevice;
    soundDevice = NULL;
    CTH_TRACE("shutdown completed\n", "audio runtime");
}

int audioRuntimeIsInitialized() {
    return (audioBuffer != NULL) || (audioProcessor != NULL) || (soundDevice != NULL);
}

AudioInputProcessor* audioRuntimeProcessor() {
    return audioProcessor;
}

AudioFrame* audioRuntimeCurrentFrame() {
    return audioFrameBuilder ? &audioRuntimeFrame : NULL;
}

int audioRuntimeIsComplete() {
    return audioRuntimeComplete.load();
}

static long long audioRuntimeDecodedSamplePosition() {
    return audioBuffer ? audioBuffer->decodedEndPosition() : 0;
}

static long long audioRuntimeSubmittedSamplePosition() {
    return audioBuffer ? audioBuffer->submittedEndPosition() : 0;
}

static long long audioRuntimeAudibleSamplePosition() {
    if ((audioBuffer == NULL) || (audioOutput == NULL))
        return 0;

    return audioOutput->audibleSamplePosition(*audioBuffer);
}
