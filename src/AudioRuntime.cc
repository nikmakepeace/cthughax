#include "cthugha.h"
#include "AudioRuntime.h"

static AudioProcessor* audioProcessor = NULL;
static AudioInput* audioInput = NULL;
static AudioOutput* audioOutput = NULL;
static AudioBuffer* audioBuffer = NULL;
static AudioFrameBuilder* audioFrameBuilder = NULL;
static char* audioRuntimeChunk = NULL;
static AudioFrame audioRuntimeFrame;
static int audioRuntimeChunkSize = 0;
static int audioRuntimeTargetLatencyMs = 250;
static int audioRuntimeInputFinished = 0;
static int audioRuntimeComplete = 0;

static const char* audioRuntimeContextName(RuntimeSoundInputContext context) {
    return (context == RSIC_FileChild) ? "file child" : "main process";
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

    int freeBytes = audioBuffer->freeSpace();
    int limitedBytes = (maxBytes < freeBytes) ? maxBytes : freeBytes;
    int bytesWanted = (audioRuntimeChunkSize < limitedBytes) ? audioRuntimeChunkSize : limitedBytes;
    bytesWanted -= bytesWanted % bytesPerSample;
    if (bytesWanted <= 0)
        return 0;

    int samplesWanted = bytesWanted / bytesPerSample;
    int samplesRead = audioInput->read(audioRuntimeChunk, bytesWanted, samplesWanted);
    int bytesRead = samplesRead * bytesPerSample;
    if (bytesRead > 0) {
        int buffered = audioBuffer->write(audioRuntimeChunk, bytesRead);
        CTH_TRACE("audio runtime: input produced samples=%d bytes=%d buffered=%d available=%d decoded-byte=%lld\n",
            samplesRead, bytesRead, buffered, audioBuffer->available(),
            audioBuffer->decoderWritePosition());
        return buffered;
    }

    if (audioInput->isFinished()) {
        audioRuntimeInputFinished = 1;
        CTH_TRACE("audio runtime: input finished decoded-byte=%lld available=%d\n",
            audioBuffer->decoderWritePosition(), audioBuffer->available());
    }

    return 0;
}

static int audioRuntimeWriteFromBuffer() {
    if ((audioBuffer == NULL) || (audioOutput == NULL) || (audioRuntimeChunk == NULL))
        return 0;

    int bytes = audioBuffer->read(audioRuntimeChunk, audioRuntimeChunkSize);
    if (bytes <= 0)
        return 0;

    int written = audioOutput->write(audioRuntimeChunk, bytes);
    CTH_TRACE("audio runtime: output drained bytes=%d written=%d available=%d output-byte=%lld audible-byte=%lld delay=%d\n",
        bytes, written, audioBuffer->available(), audioBuffer->outputReadPosition(),
        audioRuntimeAudibleBytePosition(), audioOutput->outputDelayBytes());
    return written;
}

static void audioRuntimePumpPipeline() {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL))
        return;

    if (audioRuntimeComplete)
        return;

    int bytesPerSecond = audioRuntimeBytesPerSecond();
    int targetDelay = (bytesPerSecond * audioRuntimeTargetLatencyMs) / 1000;
    if (targetDelay < audioRuntimeChunkSize)
        targetDelay = audioRuntimeChunkSize;

    if (!audioOutput->isRealtime()) {
        if (audioBuffer->available() < audioRuntimeChunkSize)
            audioRuntimeFillBuffer(audioRuntimeChunkSize);
        audioRuntimeWriteFromBuffer();
        if (audioRuntimeInputFinished && (audioBuffer->available() == 0))
            audioRuntimeComplete = 1;
    } else {
        int loops = 0;
        while (!audioRuntimeInputFinished && (audioOutput->outputDelayBytes() < targetDelay)
            && (loops < 16)) {
            if (audioBuffer->available() < audioRuntimeChunkSize)
                audioRuntimeFillBuffer(audioRuntimeChunkSize);

            if (audioRuntimeWriteFromBuffer() <= 0)
                break;
            loops++;
        }

        if (audioRuntimeInputFinished) {
            loops = 0;
            while ((audioBuffer->available() > 0) && (loops < 16)) {
                if (audioRuntimeWriteFromBuffer() <= 0)
                    break;
                loops++;
            }
            if ((audioBuffer->available() == 0) && (audioOutput->outputDelayBytes() == 0))
                audioRuntimeComplete = 1;
        }
    }

    if (audioRuntimeComplete) {
        CTH_INFO("Stopping...\n");
        CTH_TRACE("audio runtime: playback complete decoded-byte=%lld output-byte=%lld audible-byte=%lld\n",
            audioRuntimeDecodedBytePosition(), audioRuntimeOutputBytePosition(),
            audioRuntimeAudibleBytePosition());
        cthugha_close++;
    }
}

static void audioRuntimeBuildFrame() {
    if ((audioFrameBuilder == NULL) || (audioBuffer == NULL))
        return;

    audioFrameBuilder->build(audioRuntimeFrame, *audioBuffer, audioRuntimeAudibleBytePosition());
}

void audioRuntimeInit(RuntimeSoundInputContext context, int initializeInputControls) {
    CTH_TRACE("audio runtime: init requested context=%s initialize-input-controls=%d\n",
        audioRuntimeContextName(context), initializeInputControls);

    if (audioRuntimeIsInitialized()) {
        CTH_TRACE("audio runtime: init skipped because audio is already installed\n");
        return;
    }

    Settings settings = Settings::fromCurrentOptions();
    Environment environment = Environment::detect();
    RuntimeFactory runtimeFactory(settings, environment);
    AudioSourceStrategy sourceStrategy = runtimeFactory.selectAudioSourceStrategy();
    audioRuntimeInputFinished = 0;
    audioRuntimeComplete = 0;

    if (sourceStrategy == ASS_WavFile) {
        audioInput = runtimeFactory.createAudioInput();
        if ((audioInput == NULL) || audioInput->hasError()) {
            CTH_TRACE("audio runtime: native WAV input construction failed\n");
            delete audioInput;
            audioInput = NULL;
            exit(0);
        }

        audioOutput = runtimeFactory.createAudioOutput();
        if ((audioOutput == NULL) || !audioOutput->isOpen()) {
            CTH_TRACE("audio runtime: native WAV output construction failed\n");
            delete audioInput;
            audioInput = NULL;
            delete audioOutput;
            audioOutput = NULL;
            exit(0);
        }

        audioRuntimeChunkSize = audioRuntimeStrategyChunkSize();
        audioBuffer = new AudioBuffer(audioRuntimeStrategyBufferSize(), audioRuntimeStrategyHistorySize());
        audioFrameBuilder = new AudioFrameBuilder();
        audioRuntimeChunk = new char[audioRuntimeChunkSize];
        audioRuntimeFrame.clear();
        CTH_TRACE("audio runtime: installed native WAV pipeline input=%p buffer=%p output=%p frame-builder=%p chunk=%d target-latency-ms=%d\n",
            audioInput, audioBuffer, audioOutput, audioFrameBuilder, audioRuntimeChunkSize,
            audioRuntimeTargetLatencyMs);
    } else {
        audioProcessor = runtimeFactory.createAudioProcessor();
        if (audioProcessor != NULL) {
            CTH_TRACE("audio runtime: installed native AudioProcessor path\n");
            if (initializeInputControls && audioProcessor->audioInput()->initInputControls()) {
                CTH_TRACE("audio runtime: native input control initialization failed\n");
                exit(0);
            }
        } else {
            CTH_TRACE("audio runtime: native AudioProcessor unavailable; using legacy SoundDevice path\n");
            SoundDevice::install(runtimeFactory.createLegacySoundDevice(context), initializeInputControls);
        }
    }

    CTH_TRACE("audio runtime: init completed context=%s\n", audioRuntimeContextName(context));
}

void audioRuntimeTick() {
    if (audioBuffer != NULL) {
        audioRuntimePumpPipeline();
        audioRuntimeBuildFrame();
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
    CTH_TRACE("audio runtime: shutdown requested pipeline=%d native=%d legacy=%d\n",
        audioBuffer != NULL, audioProcessor != NULL, soundDevice != NULL);
    delete[] audioRuntimeChunk;
    audioRuntimeChunk = NULL;

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

    delete soundDevice;
    soundDevice = NULL;
    CTH_TRACE("audio runtime: shutdown completed\n");
}

int audioRuntimeIsInitialized() {
    return (audioBuffer != NULL) || (audioProcessor != NULL) || (soundDevice != NULL);
}

AudioProcessor* audioRuntimeProcessor() {
    return audioProcessor;
}

AudioFrame* audioRuntimeCurrentFrame() {
    return audioFrameBuilder ? &audioRuntimeFrame : NULL;
}

int audioRuntimeIsComplete() {
    return audioRuntimeComplete;
}

long long audioRuntimeDecodedBytePosition() {
    return audioBuffer ? audioBuffer->decoderWritePosition() : 0;
}

long long audioRuntimeOutputBytePosition() {
    return audioBuffer ? audioBuffer->outputReadPosition() : 0;
}

long long audioRuntimeAudibleBytePosition() {
    long long bytePosition;
    int delay;

    if ((audioBuffer == NULL) || (audioOutput == NULL))
        return 0;

    bytePosition = audioBuffer->outputReadPosition();
    delay = audioOutput->outputDelayBytes();
    if (delay > bytePosition)
        return 0;

    return bytePosition - delay;
}

int audioRuntimeReadAt(long long bytePosition, char* dst, int bytes) {
    if (audioBuffer == NULL)
        return 0;

    return audioBuffer->readAt(bytePosition, dst, bytes);
}
