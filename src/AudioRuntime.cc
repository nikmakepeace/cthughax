#include "cthugha.h"
#include "AudioRuntime.h"

static AudioProcessor* audioProcessor = NULL;
static AudioInput* audioInput = NULL;
static AudioOutput* audioOutput = NULL;
static AudioBuffer* audioBuffer = NULL;
static char* audioRuntimeChunk = NULL;
static int audioRuntimeChunkSize = 16384;
static int audioRuntimeTargetLatencyMs = 250;

static const char* audioRuntimeContextName(RuntimeSoundInputContext context) {
    return (context == RSIC_FileChild) ? "file child" : "main process";
}

static int audioRuntimeBytesPerSample() {
    return (soundFormat < 2) ? int(soundChannels) : 2 * int(soundChannels);
}

static int audioRuntimeFillBuffer(int maxBytes) {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL))
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
        CTH_TRACE("audio runtime: input produced samples=%d bytes=%d buffered=%d total-buffered=%d\n",
            samplesRead, bytesRead, buffered, audioBuffer->available());
        return buffered;
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
    CTH_TRACE("audio runtime: output drained bytes=%d written=%d buffered=%d delay=%d\n",
        bytes, written, audioBuffer->available(), audioOutput->outputDelayBytes());
    return written;
}

static void audioRuntimePumpPipeline() {
    if ((audioInput == NULL) || (audioOutput == NULL) || (audioBuffer == NULL)
        || (audioRuntimeChunk == NULL))
        return;

    int bytesPerSecond = int(soundSampleRate) * audioRuntimeBytesPerSample();
    int targetDelay = (bytesPerSecond * audioRuntimeTargetLatencyMs) / 1000;
    if (targetDelay < audioRuntimeChunkSize)
        targetDelay = audioRuntimeChunkSize;

    if (!audioOutput->isRealtime()) {
        audioRuntimeFillBuffer(audioRuntimeChunkSize);
        audioRuntimeWriteFromBuffer();
        return;
    }

    int loops = 0;
    while ((audioOutput->outputDelayBytes() < targetDelay) && (loops < 16)) {
        if (audioBuffer->available() < audioRuntimeChunkSize)
            audioRuntimeFillBuffer(audioRuntimeChunkSize);

        if (audioRuntimeWriteFromBuffer() <= 0)
            break;
        loops++;
    }
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

        audioBuffer = new AudioBuffer(64 * 1024);
        audioRuntimeChunk = new char[audioRuntimeChunkSize];
        CTH_TRACE("audio runtime: installed native WAV pipeline input=%p buffer=%p output=%p\n",
            audioInput, audioBuffer, audioOutput);
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

    delete audioOutput;
    audioOutput = NULL;

    delete audioInput;
    audioInput = NULL;

    delete audioProcessor;
    audioProcessor = NULL;

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
