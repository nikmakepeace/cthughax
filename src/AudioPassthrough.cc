/** @file
 * Optional decoded-audio passthrough output.
 */

#include "AudioPassthrough.h"
#include "ProcessServices.h"

#include <chrono>

AudioPassthrough::AudioPassthrough(AudioOutput* output,
    DecodedAudioHistory& history, const std::atomic<int>& inputFinished_,
    LogSink& log_)
    : outputValue(output)
    , playbackClockValue()
    , streamValue(history, &playbackClockValue)
    , inputFinished(inputFinished_)
    , log(log_)
    , scratch(NULL)
    , scratchSamplesValue(0)
    , stopRequested(0)
    , completeValue(0)
    , callbackDrainStarted(0)
    , outputThreadStarted(0) { }

AudioPassthrough::~AudioPassthrough() {
    stop();
    delete[] scratch;
    scratch = NULL;
}

int AudioPassthrough::start(int samplesPerSecond, int bytesPerSample,
    int inputChunkSamples, int startWorkerThread) {
    if ((outputValue.get() == NULL) || !outputValue->isOpen())
        return 1;

    outputValue->configureTiming(samplesPerSecond, bytesPerSample,
        inputChunkSamples);
    playbackClockValue.publishPresentationDelaySamples(
        outputValue->presentationDelaySamples());
    scratchSamplesValue = outputValue->scratchSamples();
    if (scratchSamplesValue <= 0)
        scratchSamplesValue = inputChunkSamples;
    if (scratchSamplesValue <= 0)
        scratchSamplesValue = 1024;

    delete[] scratch;
    scratch = new char[pcmBytesForSamples(scratchSamplesValue, bytesPerSample)];
    streamValue.reset(0);
    stopRequested.store(0);
    completeValue.store(0);

    if (outputValue->supportsCallbackDrain()) {
        callbackDrainStarted.store(1);
        outputValue->startCallbackDrain(streamValue, &inputFinished,
            scratchSamplesValue);
    } else {
        callbackDrainStarted.store(0);
    }

    if (startWorkerThread && !callbackDrainStarted.load()) {
        outputThread = std::thread(&AudioPassthrough::outputThreadMain, this);
        outputThreadStarted = 1;
    }

    log.debug("audio passthrough: started realtime=%d scratch-samples=%d target-queue-samples=%d callback-drain=%d worker-thread=%d\n",
        outputValue->isRealtime(), scratchSamplesValue,
        outputValue->queuedTargetSamples(), callbackDrainStarted.load(),
        outputThreadStarted);
    return 0;
}

void AudioPassthrough::stop() {
    stopRequested.store(1);

    if ((outputValue.get() != NULL) && callbackDrainStarted.load())
        outputValue->stopCallbackDrain();

    if (outputThread.joinable())
        outputThread.join();

    outputThreadStarted = 0;
    callbackDrainStarted.store(0);
}

int AudioPassthrough::serviceOnce() {
    if ((outputValue.get() == NULL) || (scratch == NULL))
        return 0;

    if (completeValue.load())
        return 0;

    int writeCalls = outputValue->service(streamValue, scratch,
        scratchSamplesValue, inputFinished.load());

    if (outputValue->playbackComplete(streamValue, inputFinished.load()))
        completeValue.store(1);

    return writeCalls;
}

void AudioPassthrough::notifyDecodedPcm() {
    if ((outputValue.get() != NULL) && callbackDrainStarted.load())
        outputValue->notifyCallbackDrain();
}

int AudioPassthrough::enabled() const {
    return (outputValue.get() != NULL) && outputValue->isOpen();
}

int AudioPassthrough::complete() const {
    if (!enabled())
        return 1;
    if (callbackDrainStarted.load()
        && outputValue->callbackDrainComplete(streamValue, inputFinished.load()))
        return 1;
    return completeValue.load();
}

int AudioPassthrough::providesPresentationClock() const {
    return enabled() && outputValue->isRealtime();
}

long long AudioPassthrough::presentationSamplePosition() const {
    if (!enabled())
        return 0;
    return playbackClockValue.presentationCenterSample();
}

long long AudioPassthrough::submittedSamplePosition() const {
    return playbackClockValue.submittedEndPosition();
}

int AudioPassthrough::queuedSamples() const {
    return streamValue.queuedForOutputSamples();
}

int AudioPassthrough::targetDelaySamples() const {
    return enabled() ? outputValue->queuedTargetSamples() : 0;
}

void AudioPassthrough::outputThreadMain() {
    int primed = 0;

    log.debug("audio passthrough: output thread started scratch-samples=%d target-queue-samples=%d\n",
        scratchSamplesValue, targetDelaySamples());

    while (!stopRequested.load() && !completeValue.load()) {
        if (!primed) {
            int queued = streamValue.queuedForOutputSamples();
            int prime = targetDelaySamples();
            if (!inputFinished.load() && (queued < prime)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            primed = 1;
            log.debug("audio passthrough: output thread primed queued-samples=%d prime-samples=%d input-finished=%d\n",
                queued, prime, inputFinished.load());
        }

        int writes = serviceOnce();
        if (writes <= 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    log.debug("audio passthrough: output thread stopped stop=%d complete=%d queued-samples=%d submitted-end-sample=%lld\n",
        stopRequested.load(), completeValue.load(), queuedSamples(),
        submittedSamplePosition());
}
