#include "cthugha.h"
#include "AudioRuntime.h"
#include "CthughaDisplay.h"

static AudioInputProcessor* audioProcessor = NULL;
static AudioInput* audioInput = NULL;
static AudioOutput* audioOutput = NULL;
static AudioBuffer* audioBuffer = NULL;
static AudioFrameBuilder* audioFrameBuilder = NULL;
static char* audioRuntimeChunk = NULL;
static char* audioRuntimeOutputChunk = NULL;
static AudioFrame audioRuntimeFrame;
static int audioRuntimeChunkSize = 0;
static int audioRuntimeOutputChunkSize = 0;
static int audioRuntimeInputFinished = 0;
static int audioRuntimeComplete = 0;

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

static int audioRuntimeAlignBytes(int bytes) {
    int bytesPerSample = audioRuntimeBytesPerSample();

    if (bytesPerSample <= 0)
        return bytes;

    bytes -= bytes % bytesPerSample;
    return bytes;
}

static int audioRuntimeBytesPerSecond() {
    return int(soundSampleRate) * audioRuntimeBytesPerSample();
}

static int audioRuntimeStrategyChunkSize() {
    int bytes = audioRuntimeBytesPerSecond() / 20;

    if (bytes < 4096)
        bytes = 4096;
    if (bytes > 32768)
        bytes = 32768;

    return audioRuntimeAlignBytes(bytes);
}

static int audioRuntimeStrategyBufferSize() {
    int bytes = audioRuntimeBytesPerSecond() * 3;

    if (bytes < 64 * 1024)
        bytes = 64 * 1024;

    return audioRuntimeAlignBytes(bytes);
}

static int audioRuntimeStrategyHistorySize() {
    int bytes = audioRuntimeBytesPerSecond();

    if (bytes < 32 * 1024)
        bytes = 32 * 1024;

    return audioRuntimeAlignBytes(bytes);
}

static int audioRuntimeFillBuffer(int maxBytes) {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL))
        return 0;
    if (audioRuntimeInputFinished)
        return 0;

    int bytesPerSample = audioRuntimeBytesPerSample();
    if (bytesPerSample <= 0)
        return 0;

    int freeBytes = audioBuffer->writableBytes();
    int limitedBytes = (maxBytes < freeBytes) ? maxBytes : freeBytes;
    int bytesWanted = (audioRuntimeChunkSize < limitedBytes) ? audioRuntimeChunkSize : limitedBytes;
    bytesWanted -= bytesWanted % bytesPerSample;
    if (bytesWanted <= 0)
        return 0;

    int samplesWanted = bytesWanted / bytesPerSample;
    double readStart = getTime();
    int samplesRead = audioInput->read(audioRuntimeChunk, bytesWanted, samplesWanted);
    double readEnd = getTime();
    int bytesRead = samplesRead * bytesPerSample;
    if (bytesRead > 0) {
        double bufferStart = getTime();
        int buffered = audioBuffer->appendDecodedPcm(audioRuntimeChunk, bytesRead);
        double bufferEnd = getTime();
        CTH_TRACE("input produced samples=%d bytes=%d appended=%d queued=%d decoded-end-byte=%lld\n", "audio runtime",
            samplesRead, bytesRead, buffered, audioBuffer->queuedForOutputBytes(),
            audioBuffer->decodedEndPosition());
        CTH_TRACE("fill read-ms=%.3f buffer-write-ms=%.3f bytes=%d queued=%d\n", "audio timing",
            (readEnd - readStart) * 1000.0, (bufferEnd - bufferStart) * 1000.0, bytesRead,
            audioBuffer->queuedForOutputBytes());
        return buffered;
    }

    if (audioInput->isFinished()) {
        audioRuntimeInputFinished = 1;
        CTH_TRACE("input finished decoded-end-byte=%lld queued=%d\n", "audio runtime",
            audioBuffer->decodedEndPosition(), audioBuffer->queuedForOutputBytes());
    }

    return 0;
}

static void audioRuntimePumpPipeline() {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL) || (audioRuntimeOutputChunk == NULL))
        return;

    if (audioRuntimeComplete)
        return;

    double pumpStart = getTime();
    int fillCalls = 0;
    int targetDelay = audioOutput->targetDelayBytes();

    int inputTarget = audioOutput->queuedTargetBytes();
    int loops = 0;
    while (!audioRuntimeInputFinished && (audioBuffer->queuedForOutputBytes() < inputTarget)
        && (loops < 16)) {
        if (audioRuntimeFillBuffer(audioRuntimeChunkSize) <= 0)
            break;
        fillCalls++;
        loops++;
    }

    int writeCalls = audioOutput->service(*audioBuffer, audioRuntimeOutputChunk,
        audioRuntimeOutputChunkSize, audioRuntimeInputFinished);

    if (audioOutput->playbackComplete(*audioBuffer, audioRuntimeInputFinished))
        audioRuntimeComplete = 1;

    CTH_TRACE("pump total-ms=%.3f fills=%d writes=%d queued=%d delay=%d target-delay=%d complete=%d\n", "audio timing",
        (getTime() - pumpStart) * 1000.0, fillCalls, writeCalls,
        audioBuffer->queuedForOutputBytes(), audioOutput->outputDelayBytes(), targetDelay, audioRuntimeComplete);

    if (audioRuntimeComplete) {
        CTH_INFO("Stopping...\n");
        CTH_TRACE("playback complete decoded-end-byte=%lld submitted-end-byte=%lld audible-byte=%lld\n", "audio runtime",
            audioRuntimeDecodedBytePosition(), audioRuntimeSubmittedBytePosition(),
            audioRuntimeAudibleBytePosition());
        cthugha_close++;
    }
}

static void audioRuntimeBuildFrame() {
    if ((audioFrameBuilder == NULL) || (audioBuffer == NULL))
        return;

    long long audibleByte = audioRuntimeAudibleBytePosition();
    double visualLatencySeconds = cthughaDisplay ? cthughaDisplay->visualLatencySeconds() : 0;
    long long visualOffsetBytes = (long long)(audioRuntimeBytesPerSecond() * visualLatencySeconds);
    int bytesPerSample = audioRuntimeBytesPerSample();
    if (bytesPerSample > 0)
        visualOffsetBytes -= visualOffsetBytes % bytesPerSample;

    long long frameCenterByte = audibleByte + visualOffsetBytes;
    if (frameCenterByte > audioBuffer->decodedEndPosition())
        frameCenterByte = audioBuffer->decodedEndPosition();

    double buildStart = getTime();
    audioFrameBuilder->build(audioRuntimeFrame, *audioBuffer, frameCenterByte);
    CTH_TRACE("frame-build-ms=%.3f audible-byte=%lld visual-offset-bytes=%lld visual-latency-ms=%.3f center-byte=%lld queued=%d\n",
        "audio timing",
        (getTime() - buildStart) * 1000.0, audibleByte, visualOffsetBytes,
        visualLatencySeconds * 1000.0, frameCenterByte, audioBuffer->queuedForOutputBytes());
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
    audioRuntimeInputFinished = 0;
    audioRuntimeComplete = 0;

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

        audioRuntimeChunkSize = audioRuntimeStrategyChunkSize();
        audioOutput->configureTiming(audioRuntimeBytesPerSecond(), audioRuntimeChunkSize);
        audioRuntimeOutputChunkSize = audioOutput->scratchBytes();
        audioBuffer = new AudioBuffer(audioRuntimeStrategyBufferSize(), audioRuntimeStrategyHistorySize());
        audioFrameBuilder = new AudioFrameBuilder();
        audioRuntimeChunk = new char[audioRuntimeChunkSize];
        audioRuntimeOutputChunk = new char[audioRuntimeOutputChunkSize];
        audioRuntimeFrame.clear();
        CTH_TRACE("installed native file pipeline strategy=%d input=%p buffer=%p output=%p frame-builder=%p input-chunk=%d output-chunk=%d target-delay=%d\n", "audio runtime",
            sourceStrategy, audioInput, audioBuffer, audioOutput, audioFrameBuilder,
            audioRuntimeChunkSize, audioRuntimeOutputChunkSize, audioOutput->targetDelayBytes());
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
        audioRuntimePumpPipeline();
        double pumpEnd = getTime();
        audioRuntimeBuildFrame();
        double buildEnd = getTime();
        CTH_TRACE("tick pipeline-ms=%.3f pump-ms=%.3f build-ms=%.3f queued=%d delay=%d decoded-end-byte=%lld submitted-end-byte=%lld audible-byte=%lld\n",
            "audio timing",
            (buildEnd - tickStart) * 1000.0,
            (pumpEnd - pumpStart) * 1000.0,
            (buildEnd - pumpEnd) * 1000.0,
            audioBuffer->queuedForOutputBytes(),
            audioOutput ? audioOutput->outputDelayBytes() : 0,
            audioRuntimeDecodedBytePosition(),
            audioRuntimeSubmittedBytePosition(),
            audioRuntimeAudibleBytePosition());
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

    audioRuntimeInputFinished = 0;
    audioRuntimeComplete = 0;
    audioRuntimeChunkSize = 0;
    audioRuntimeOutputChunkSize = 0;

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
    return audioRuntimeComplete;
}

long long audioRuntimeDecodedBytePosition() {
    return audioBuffer ? audioBuffer->decodedEndPosition() : 0;
}

long long audioRuntimeSubmittedBytePosition() {
    return audioBuffer ? audioBuffer->submittedEndPosition() : 0;
}

long long audioRuntimeAudibleBytePosition() {
    if ((audioBuffer == NULL) || (audioOutput == NULL))
        return 0;

    return audioOutput->audibleBytePosition(*audioBuffer);
}

int audioRuntimeReadProtectedPcmAt(long long bytePosition, char* dst, int bytes) {
    if (audioBuffer == NULL)
        return 0;

    return audioBuffer->readProtectedPcmAt(bytePosition, dst, bytes);
}
