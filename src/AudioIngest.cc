/** @file
 * Application-owned audio acquisition and visual-frame production.
 */

#include "AudioIngest.h"
#include "ProcessServices.h"
#ifndef CTH_AUDIO_INGEST_NO_CONFIG
#include "AudioSettings.h"
#include "RuntimeFactory.h"
#endif

#include <chrono>

static int audioIngestChunkSamples(int samplesPerSecond) {
    int samples = samplesPerSecond / 20;

    if (samples < 1024)
        samples = 1024;
    if (samples > 8192)
        samples = 8192;

    return samples;
}

static int audioIngestHistorySamples(int samplesPerSecond,
    int passthroughDelaySamples) {
    int samples = passthroughDelaySamples + 1024;
    int minimumSamples = samplesPerSecond;

    if (minimumSamples < 16384)
        minimumSamples = 16384;
    if (samples < minimumSamples)
        samples = minimumSamples;

    return samples;
}

static int audioIngestCapacitySamples(int samplesPerSecond,
    int retainedHistorySamples, int decodeAheadSamples) {
    int samples = retainedHistorySamples + decodeAheadSamples;
    int minimumSamples = samplesPerSecond * 3;

    if (minimumSamples < 32768)
        minimumSamples = 32768;
    if (samples < minimumSamples)
        samples = minimumSamples;

    return samples;
}

AudioIngest::AudioIngest(const AudioConfig& config, int visualMaxDimension,
    RandomSource& randomSource_, SecondsClock& clock_, LogSink& log_,
    int startWorkerThreads)
    : configValue(config)
    , hasConfig(1)
    , visualMaxDimensionValue(visualMaxDimension)
    , startWorkerThreadsValue(startWorkerThreads)
    , autoCloseOnInputFinishedValue(config.inputMode == AIM_File)
    , randomSource(&randomSource_)
    , clock(&clock_)
    , log(&log_)
    , frameBuilder(log_)
    , initializedValue(0)
    , inputChunkSamplesValue(0)
    , decodeAheadSamplesValue(0)
    , samplesPerSecondValue(0)
    , bytesPerSampleValue(0)
    , visualClockStarted(0)
    , visualClockStartSeconds(0.0)
    , inputFinished(0)
    , stopRequested(0)
    , inputThreadStarted(0) { }

AudioIngest::AudioIngest(AudioInput* input, AudioOutput* output,
    int visualMaxDimension, SecondsClock& clock_,
    LogSink& log_, int autoCloseOnInputFinished, int startWorkerThreads)
    : configValue()
    , hasConfig(0)
    , visualMaxDimensionValue(visualMaxDimension)
    , startWorkerThreadsValue(startWorkerThreads)
    , autoCloseOnInputFinishedValue(autoCloseOnInputFinished)
    , randomSource(NULL)
    , clock(&clock_)
    , log(&log_)
    , inputValue(input)
    , injectedOutputValue(output)
    , frameBuilder(log_)
    , initializedValue(0)
    , inputChunkSamplesValue(0)
    , decodeAheadSamplesValue(0)
    , samplesPerSecondValue(0)
    , bytesPerSampleValue(0)
    , visualClockStarted(0)
    , visualClockStartSeconds(0.0)
    , inputFinished(0)
    , stopRequested(0)
    , inputThreadStarted(0) { }

AudioIngest::~AudioIngest() {
    stop();
}

int AudioIngest::start() {
    if (initializedValue)
        return 0;

    inputFinished.store(0);
    stopRequested.store(0);
    visualClockStarted = 0;
    visualClockStartSeconds = 0.0;
    frameValue.clear();

    if (hasConfig)
        return buildFromConfig();

    return buildFromInjectedRuntime();
}

int AudioIngest::buildFromConfig() {
#ifdef CTH_AUDIO_INGEST_NO_CONFIG
    return 1;
#else
    AudioSettings settings = AudioSettings::fromConfig(configValue, *log);
    AudioOutputConfig outputConfig = AudioOutputConfig::fromConfig(configValue);
    Environment environment = Environment::detect(settings, *log);
    outputDumpValue.reset(new AudioOutputDump(outputConfig.outputDumpPath,
        *log));
    RuntimeFactory runtimeFactory(settings, outputConfig, environment,
        visualMaxDimensionValue, outputDumpValue.get(), *randomSource, *clock,
        *log);
    std::unique_ptr<AudioOutput> output;

    inputValue.reset(runtimeFactory.createAudioInput());
    if ((inputValue.get() != NULL) && inputValue->hasError()) {
        log->debug("audio ingest: input construction failed for mode=%d\n",
            settings.audioInputMode);
        inputValue.reset();
        if (settings.audioInputMode == AIM_File)
            return 1;
        if (settings.audioInputMode == AIM_DSPIn)
            log->warn("Can not use requested sound input. Visual audio input is silent.\n");
    }

    if ((inputValue.get() == NULL) && (settings.audioInputMode == AIM_DSPIn))
        log->warn("Can not use requested sound input. Visual audio input is silent.\n");
    if ((inputValue.get() == NULL) && (settings.audioInputMode == AIM_File)) {
        log->debug("audio ingest: requested file input has no PCM source\n");
        return 1;
    }

    pcmFormatValue = (inputValue.get() != NULL)
        ? inputValue->format()
        : settings.pcmFormat;

    if ((inputValue.get() != NULL) && !settings.silent) {
        output.reset(runtimeFactory.createAudioOutput(pcmFormatValue));
        if ((output.get() == NULL) || !output->isOpen()) {
            log->debug("audio ingest: output construction failed\n");
            return settings.audioInputMode == AIM_File ? 1 : 0;
        }
    }

    return initializeRuntimeObjects(output.release(), settings.silent);
#endif
}

int AudioIngest::buildFromInjectedRuntime() {
    if (inputValue.get() != NULL) {
        pcmFormatValue = inputValue->format();
    } else {
        pcmFormatValue.sampleRate = 44100;
        pcmFormatValue.channels = 2;
        pcmFormatValue.sampleFormat = SF_s16_le;
    }

    int silentPassthrough = injectedOutputValue.get() == NULL;
    return initializeRuntimeObjects(injectedOutputValue.release(),
        silentPassthrough);
}

int AudioIngest::initializeRuntimeObjects(AudioOutput* output,
    int silentPassthrough) {
    samplesPerSecondValue = pcmFormatValue.sampleRate;
    bytesPerSampleValue = pcmFormatValue.bytesPerSample();
    if (samplesPerSecondValue <= 0)
        samplesPerSecondValue = 44100;
    if (bytesPerSampleValue <= 0)
        bytesPerSampleValue = 1;

    inputChunkSamplesValue = audioIngestChunkSamples(samplesPerSecondValue);
    decodeAheadSamplesValue = inputChunkSamplesValue * 4;

    std::unique_ptr<AudioOutput> outputHolder(output);
    int passthroughDelaySamples = 0;
    if ((outputHolder.get() != NULL) && !silentPassthrough) {
        outputHolder->configureTiming(samplesPerSecondValue, bytesPerSampleValue,
            inputChunkSamplesValue);
        passthroughDelaySamples = outputHolder->queuedTargetSamples();
        decodeAheadSamplesValue += passthroughDelaySamples;
    }

    int retainedSamples = audioIngestHistorySamples(samplesPerSecondValue,
        passthroughDelaySamples);
    int capacitySamples = audioIngestCapacitySamples(samplesPerSecondValue,
        retainedSamples, decodeAheadSamplesValue);

    historyValue.reset(new DecodedAudioHistory(capacitySamples, pcmFormatValue,
        retainedSamples, *log));
    inputChunk.reset(new char[pcmBytesForSamples(inputChunkSamplesValue,
        bytesPerSampleValue)]);

    if ((outputHolder.get() != NULL) && !silentPassthrough) {
        passthroughValue.reset(new AudioPassthrough(outputHolder.release(),
            *historyValue, inputFinished, *log));
        if (passthroughValue->start(samplesPerSecondValue, bytesPerSampleValue,
                inputChunkSamplesValue, startWorkerThreadsValue))
            return 1;
    }

    if (inputValue.get() == NULL)
        inputFinished.store(1);

    initializedValue = 1;
    if (startWorkerThreadsValue && (inputValue.get() != NULL)) {
        inputThread = std::thread(&AudioIngest::inputThreadMain, this);
        inputThreadStarted = 1;
    }

    if (log != NULL) {
        log->debug("audio ingest: started input=%p passthrough=%p sample-rate=%d channels=%d format=%d bytes-per-sample=%d input-chunk-samples=%d decode-ahead-samples=%d retained-samples=%d capacity-samples=%d worker-thread=%d\n",
            inputValue.get(), passthroughValue.get(), samplesPerSecondValue,
            pcmFormatValue.channels, pcmFormatValue.sampleFormat,
            bytesPerSampleValue, inputChunkSamplesValue, decodeAheadSamplesValue,
            retainedSamples, capacitySamples, inputThreadStarted);
    }
    return 0;
}

void AudioIngest::stop() {
    stopRequested.store(1);

    if (passthroughValue.get() != NULL)
        passthroughValue->stop();

    if (inputThread.joinable())
        inputThread.join();

    inputThreadStarted = 0;
    passthroughValue.reset();
    inputChunk.reset();
    historyValue.reset();
    outputDumpValue.reset();
    inputValue.reset();
    injectedOutputValue.reset();
    frameValue.clear();
    initializedValue = 0;
    inputFinished.store(0);
    stopRequested.store(0);
    visualClockStarted = 0;
    visualClockStartSeconds = 0.0;
}

void AudioIngest::tick() {
    if (!initializedValue)
        return;

    int traceTick = (log != NULL) && log->traceEnabled();
    double tickStart = traceTick ? clock->nowSeconds() : 0.0;
    if (!inputThreadStarted)
        pumpInputToTarget();

    if ((passthroughValue.get() != NULL) && !startWorkerThreadsValue)
        passthroughValue->serviceOnce();

    if (historyValue.get() != NULL) {
        long long centerSample = frameCenterSamplePosition();
        long long decodedEndSample = historyValue->decodedEndPosition();
        if (centerSample > decodedEndSample)
            centerSample = decodedEndSample;
        frameBuilder.build(frameValue, *historyValue, centerSample);
    }

    if (traceTick) {
        log->trace("audio timing",
            "audio-ingest tick-ms=%.3f decoded-end-sample=%lld presentation-sample=%lld input-finished=%d complete=%d\n",
            (clock->nowSeconds() - tickStart) * 1000.0,
            decodedSamplePosition(), presentationSamplePosition(),
            inputFinished.load(), complete());
    }
}

AudioFrame& AudioIngest::currentFrame() {
    return frameValue;
}

const AudioFrame& AudioIngest::currentFrame() const {
    return frameValue;
}

const PcmFormat& AudioIngest::format() const {
    return pcmFormatValue;
}

int AudioIngest::complete() const {
    if (!autoCloseOnInputFinishedValue || !initializedValue)
        return 0;
    if (!inputFinished.load())
        return 0;

    if (passthroughValue.get() != NULL)
        return passthroughValue->complete();

    if (historyValue.get() == NULL)
        return 0;

    long long decodedEndSample = historyValue->decodedEndPosition();
    if (decodedEndSample <= 0)
        return 1;

    double start = visualClockStarted ? visualClockStartSeconds : clock->nowSeconds();
    long long visualSample = (long long)((clock->nowSeconds() - start)
        * samplesPerSecondValue);
    return visualSample >= decodedEndSample;
}

long long AudioIngest::decodedSamplePosition() const {
    return historyValue.get() ? historyValue->decodedEndPosition() : 0;
}

long long AudioIngest::presentationSamplePosition() const {
    if ((passthroughValue.get() != NULL)
        && passthroughValue->providesPresentationClock()
        && (passthroughValue->submittedSamplePosition() > 0))
        return passthroughValue->presentationSamplePosition();

    if (!visualClockStarted)
        return 0;

    long long sample = (long long)((clock->nowSeconds() - visualClockStartSeconds)
        * samplesPerSecondValue);
    return sample < 0 ? 0 : sample;
}

AudioPassthrough* AudioIngest::passthrough() {
    return passthroughValue.get();
}

DecodedAudioHistory* AudioIngest::history() {
    return historyValue.get();
}

int AudioIngest::fillInput(int maxSamples) {
    if ((inputValue.get() == NULL) || (historyValue.get() == NULL)
        || (inputChunk.get() == NULL) || inputFinished.load())
        return 0;

    int freeSamples = historyValue->writableSamples();
    int limitedSamples = (maxSamples < freeSamples) ? maxSamples : freeSamples;
    int samplesWanted = (inputChunkSamplesValue < limitedSamples)
        ? inputChunkSamplesValue : limitedSamples;
    if (samplesWanted <= 0)
        return 0;

    int samplesRead = inputValue->read(inputChunk.get(),
        pcmBytesForSamples(samplesWanted, bytesPerSampleValue), samplesWanted);
    if (samplesRead > 0) {
        int appended = historyValue->appendDecodedPcm(inputChunk.get(), samplesRead);
        if ((appended > 0) && (passthroughValue.get() != NULL))
            passthroughValue->notifyDecodedPcm();
        return appended;
    }

    if (inputValue->isFinished()) {
        inputFinished.store(1);
        if (passthroughValue.get() != NULL)
            passthroughValue->notifyDecodedPcm();
        if (log != NULL) {
            log->debug("audio ingest: input finished decoded-end-sample=%lld\n",
                decodedSamplePosition());
        }
    }

    return 0;
}

void AudioIngest::pumpInputToTarget() {
    if ((inputValue.get() == NULL) || (historyValue.get() == NULL)
        || inputFinished.load())
        return;

    long long target = decodeTargetSamplePosition();
    int loops = 0;
    while (!inputFinished.load()
        && (historyValue->decodedEndPosition() < target)
        && (loops < 16)) {
        if (fillInput(inputChunkSamplesValue) <= 0)
            break;
        loops++;
    }
}

void AudioIngest::inputThreadMain() {
    if (log != NULL)
        log->debug("audio ingest: input thread started chunk-samples=%d\n",
            inputChunkSamplesValue);

    while (!stopRequested.load() && !inputFinished.load()) {
        long long target = decodeTargetSamplePosition();
        if ((historyValue.get() != NULL)
            && (historyValue->decodedEndPosition() < target)) {
            if (fillInput(inputChunkSamplesValue) > 0)
                continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    if (log != NULL) {
        log->debug("audio ingest: input thread stopped stop=%d input-finished=%d decoded-end-sample=%lld\n",
            stopRequested.load(), inputFinished.load(), decodedSamplePosition());
    }
}

long long AudioIngest::visualClockSamplePosition() {
    if (!visualClockStarted) {
        visualClockStartSeconds = clock->nowSeconds();
        visualClockStarted = 1;
    }

    long long sample = (long long)((clock->nowSeconds() - visualClockStartSeconds)
        * samplesPerSecondValue);
    if (sample < 0)
        sample = 0;

    return sample;
}

long long AudioIngest::frameCenterSamplePosition() {
    long long sample = visualClockSamplePosition();

    if ((passthroughValue.get() != NULL)
        && passthroughValue->providesPresentationClock()
        && (passthroughValue->submittedSamplePosition() > 0))
        sample = passthroughValue->presentationSamplePosition();

    if (sample < 0)
        sample = 0;

    return sample;
}

long long AudioIngest::decodeTargetSamplePosition() {
    long long target = frameCenterSamplePosition() + decodeAheadSamplesValue;

    if (passthroughValue.get() != NULL) {
        long long passthroughTarget = passthroughValue->submittedSamplePosition()
            + passthroughValue->targetDelaySamples() + inputChunkSamplesValue * 4;
        if (target < passthroughTarget)
            target = passthroughTarget;
    }

    if (target < inputChunkSamplesValue)
        target = inputChunkSamplesValue;

    return target;
}
