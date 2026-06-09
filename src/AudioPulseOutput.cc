// PulseAudio output backend.
#include "config.h"
#include "Audio.h"
#include "AudioInternal.h"
#include "ProcessServices.h"

#if WITH_PULSE == 1
#include <pulse/pulseaudio.h>
#include <stdint.h>
#endif

#if WITH_PULSE == 1

static int audioPulseBytesToMs(unsigned int bytes, int bytesPerSecond) {
    if (bytesPerSecond <= 0)
        return 0;
    long long ms = ((long long)bytes * 1000LL) / bytesPerSecond;
    return (ms > INT_MAX) ? INT_MAX : (int)ms;
}

static int audioPulseBytesToSamples(unsigned int bytes, int bytesPerSample) {
    if (bytesPerSample <= 0)
        return 0;
    return (int)(bytes / (unsigned int)bytesPerSample);
}

static pa_sample_format_t audioPulseSampleFormat(const PcmFormat& format) {
    switch (format.sampleFormat) {
    case SF_u8:
        return PA_SAMPLE_U8;
    case SF_s16_le:
        return PA_SAMPLE_S16LE;
    case SF_s16_be:
        return PA_SAMPLE_S16BE;
    default:
        return PA_SAMPLE_INVALID;
    }
}

static void audioPulseContextStateCallback(pa_context*, void* userdata) {
    if (userdata != NULL)
        pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}

static void audioPulseStreamStateCallback(pa_stream*, void* userdata) {
    if (userdata != NULL)
        pa_threaded_mainloop_signal((pa_threaded_mainloop*)userdata, 0);
}

static void audioPulseStreamWriteCallback(pa_stream*, size_t requestedBytes, void* userdata) {
    if (userdata != NULL)
        ((AudioPulseOutput*)userdata)->pulseWritable(requestedBytes);
}

static void audioPulseStreamUnderflowCallback(pa_stream*, void* userdata) {
    if (userdata != NULL)
        ((AudioPulseOutput*)userdata)->pulseUnderflow();
}

static void audioPulseDrainDeferCallback(pa_mainloop_api* api, pa_defer_event* event,
    void* userdata) {
    if ((api != NULL) && (event != NULL))
        api->defer_enable(event, 0);
    if (userdata != NULL)
        ((AudioPulseOutput*)userdata)->pulseWritable((size_t)-1);
}

static int audioPulseWaitForContextReady(pa_threaded_mainloop* mainloop,
    pa_context* context) {
    for (;;) {
        pa_context_state_t state = pa_context_get_state(context);

        if (state == PA_CONTEXT_READY)
            return 1;
        if (!PA_CONTEXT_IS_GOOD(state))
            return 0;

        pa_threaded_mainloop_wait(mainloop);
    }
}

static int audioPulseWaitForStreamReady(pa_threaded_mainloop* mainloop,
    pa_stream* stream) {
    for (;;) {
        pa_stream_state_t state = pa_stream_get_state(stream);

        if (state == PA_STREAM_READY)
            return 1;
        if (!PA_STREAM_IS_GOOD(state))
            return 0;

        pa_threaded_mainloop_wait(mainloop);
    }
}

AudioPulseOutput::AudioPulseOutput(const PcmFormat& format, SecondsClock& clock_,
    LogSink& log_,
    const AudioOutputConfig& config, AudioOutputDump* outputDump, int autoOpen)
    : AudioOutput(config.pulseOutputTargetLatencyMs, outputDump, clock_, log_)
    , pcmFormatValue(format)
    , outputConfigValue(config)
    , mainloop(NULL)
    , context(NULL)
    , stream(NULL)
    , drainEvent(NULL)
    , callbackStream(NULL)
    , callbackInputFinished(NULL)
    , callbackScratch(NULL)
    , callbackScratchSamples(0)
    , callbackDrainActive(0)
    , bytesPerSecondValue(0)
    , pulsePresentationDelaySamples(0)
    , adaptiveQueueTargetSamples(0)
    , underflowCountValue(0)
    , lastReportedUnderflows(0) {
    if (autoOpen)
        update();
}

AudioPulseOutput::~AudioPulseOutput() {
    closePulse();
}

void AudioPulseOutput::closePulse() {
    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;

    if (pulseMainloop != NULL)
        pa_threaded_mainloop_lock(pulseMainloop);

    callbackDrainActive.store(0);
    callbackStream = NULL;
    callbackInputFinished = NULL;
    callbackScratchSamples = 0;

    if ((pulseMainloop != NULL) && (drainEvent != NULL)) {
        pa_mainloop_api* api = pa_threaded_mainloop_get_api(pulseMainloop);
        if (api != NULL)
            api->defer_free((pa_defer_event*)drainEvent);
        drainEvent = NULL;
    }

    if (stream != NULL) {
        pa_stream* pulseStream = (pa_stream*)stream;
        pa_stream_set_state_callback(pulseStream, NULL, NULL);
        pa_stream_set_write_callback(pulseStream, NULL, NULL);
        pa_stream_set_underflow_callback(pulseStream, NULL, NULL);
        pa_stream_disconnect(pulseStream);
        pa_stream_unref(pulseStream);
        stream = NULL;
    }

    if (context != NULL) {
        pa_context* pulseContext = (pa_context*)context;
        pa_context_set_state_callback(pulseContext, NULL, NULL);
        pa_context_disconnect(pulseContext);
        pa_context_unref(pulseContext);
        context = NULL;
    }

    if (pulseMainloop != NULL) {
        pa_threaded_mainloop_unlock(pulseMainloop);
        pa_threaded_mainloop_stop(pulseMainloop);
        pa_threaded_mainloop_free(pulseMainloop);
        mainloop = NULL;
    }

    bytesPerSecondValue = 0;
    adaptiveQueueTargetSamples.store(0);
    lastReportedUnderflows.store(underflowCountValue.load());
    delete[] callbackScratch;
    callbackScratch = NULL;
}

int AudioPulseOutput::defaultTargetLatencyMs() const {
    // Passthrough queue latency is separate from the Pulse stream buffer
    // latency used when opening the PulseAudio connection.
    return AudioOutput::defaultTargetLatencyMs();
}

int AudioPulseOutput::timingScratchSamples(int, int targetDelaySamples) const {
    return targetDelaySamples;
}

void AudioPulseOutput::update() {
    pa_sample_spec sampleSpec;
    pa_buffer_attr bufferAttr;

    closePulse();

    sampleSpec.format = audioPulseSampleFormat(pcmFormatValue);
    sampleSpec.rate = pcmFormatValue.sampleRate;
    sampleSpec.channels = pcmFormatValue.channels;

    if (sampleSpec.format == PA_SAMPLE_INVALID) {
        logSink().debug("    audio output strategy: Pulse unavailable for format `%s'\n",
            audioSampleFormatText(pcmFormatValue.sampleFormat));
        return;
    }

    int bytesPerSampleValue = pcmFormatValue.bytesPerSample();
    int targetSamples = (sampleSpec.rate * outputConfigValue.pulseLatencyMs) / 1000;
    int minRequestSamples = sampleSpec.rate / 20;
    if (minRequestSamples < 1)
        minRequestSamples = 1;
    if (targetSamples < minRequestSamples)
        targetSamples = minRequestSamples;

    unsigned int targetBytes = (unsigned int)pcmBytesForSamples(targetSamples, bytesPerSampleValue);
    unsigned int minRequestBytes = (unsigned int)pcmBytesForSamples(minRequestSamples, bytesPerSampleValue);
    unsigned int prebufferBytes = targetBytes;
    if (prebufferBytes > minRequestBytes)
        prebufferBytes -= minRequestBytes;

    bufferAttr.maxlength = targetBytes * 2;
    bufferAttr.tlength = targetBytes;
    bufferAttr.prebuf = prebufferBytes;
    bufferAttr.minreq = minRequestBytes;
    bufferAttr.fragsize = (uint32_t)-1;

    logSink().debug("    audio output strategy: Pulse latency request configured-latency-ms=%d target-samples=%d target-bytes=%u prebuffer-bytes=%u min-request-samples=%d min-request-bytes=%u\n",
        outputConfigValue.pulseLatencyMs, targetSamples, targetBytes,
        prebufferBytes, minRequestSamples, minRequestBytes);
    logSink().debug("    audio output strategy: opening threaded Pulse server `%s'\n",
        serverDisplayName());

    mainloop = pa_threaded_mainloop_new();
    if (mainloop == NULL) {
        logSink().debug("    audio output strategy: Pulse server `%s' failed to open: no threaded mainloop\n",
            serverDisplayName());
        return;
    }

    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;
    if (pa_threaded_mainloop_start(pulseMainloop) < 0) {
        logSink().debug("    audio output strategy: Pulse server `%s' failed to open: could not start threaded mainloop\n",
            serverDisplayName());
        pa_threaded_mainloop_free(pulseMainloop);
        mainloop = NULL;
        return;
    }

    pa_threaded_mainloop_lock(pulseMainloop);

    pa_mainloop_api* api = pa_threaded_mainloop_get_api(pulseMainloop);
    drainEvent = api->defer_new(api, audioPulseDrainDeferCallback, this);
    if (drainEvent != NULL)
        api->defer_enable((pa_defer_event*)drainEvent, 0);

    context = pa_context_new(api, "Cthughanix");
    if (context == NULL) {
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' failed to open: no context\n",
            serverDisplayName());
        closePulse();
        return;
    }

    pa_context* pulseContext = (pa_context*)context;
    pa_context_set_state_callback(pulseContext, audioPulseContextStateCallback,
        pulseMainloop);
    if (pa_context_connect(pulseContext, serverName(), PA_CONTEXT_NOFLAGS,
            NULL)
        < 0) {
        const char* errorText = pa_strerror(pa_context_errno(pulseContext));
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' failed to connect: %s\n",
            serverDisplayName(), errorText);
        closePulse();
        return;
    }

    if (!audioPulseWaitForContextReady(pulseMainloop, pulseContext)) {
        const char* errorText = pa_strerror(pa_context_errno(pulseContext));
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' failed to become ready: %s\n",
            serverDisplayName(), errorText);
        closePulse();
        return;
    }

    stream = pa_stream_new(pulseContext, "Audio passthrough", &sampleSpec, NULL);
    if (stream == NULL) {
        const char* errorText = pa_strerror(pa_context_errno(pulseContext));
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' failed to create stream: %s\n",
            serverDisplayName(), errorText);
        closePulse();
        return;
    }

    pa_stream* pulseStream = (pa_stream*)stream;
    pa_stream_set_state_callback(pulseStream, audioPulseStreamStateCallback,
        pulseMainloop);
    pa_stream_set_write_callback(pulseStream, audioPulseStreamWriteCallback,
        this);
    pa_stream_set_underflow_callback(pulseStream, audioPulseStreamUnderflowCallback,
        this);

    pa_stream_flags_t flags = PA_STREAM_NOFLAGS;
    if (pa_stream_connect_playback(pulseStream, NULL, &bufferAttr, flags, NULL,
            NULL)
        < 0) {
        const char* errorText = pa_strerror(pa_context_errno(pulseContext));
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' failed to connect stream: %s\n",
            serverDisplayName(), errorText);
        closePulse();
        return;
    }

    if (!audioPulseWaitForStreamReady(pulseMainloop, pulseStream)) {
        const char* errorText = pa_strerror(pa_context_errno(pulseContext));
        pa_threaded_mainloop_unlock(pulseMainloop);
        logSink().debug("    audio output strategy: Pulse server `%s' stream failed to become ready: %s\n",
            serverDisplayName(), errorText);
        closePulse();
        return;
    }

    bytesPerSecondValue = pa_bytes_per_second(&sampleSpec);
    const pa_buffer_attr* actualBufferAttr = pa_stream_get_buffer_attr(pulseStream);
    unsigned int actualTarget = actualBufferAttr ? actualBufferAttr->tlength : bufferAttr.tlength;
    unsigned int actualPrebuf = actualBufferAttr ? actualBufferAttr->prebuf : bufferAttr.prebuf;
    unsigned int actualMinRequest = actualBufferAttr ? actualBufferAttr->minreq : bufferAttr.minreq;
    unsigned int actualMax = actualBufferAttr ? actualBufferAttr->maxlength : bufferAttr.maxlength;
    int presentationDelaySamples = audioPulseBytesToSamples(actualTarget,
        bytesPerSampleValue);
    if (presentationDelaySamples <= 0)
        presentationDelaySamples = targetSamples;
    pulsePresentationDelaySamples.store(presentationDelaySamples);

    pa_threaded_mainloop_unlock(pulseMainloop);

    logSink().debug("    audio output strategy: threaded Pulse server `%s' connected successfully\n",
        serverDisplayName());
    logSink().debug("    audio output strategy: Pulse buffer target=%u bytes (%d ms) prebuffer=%u bytes (%d ms) min-request=%u bytes (%d ms) max=%u bytes (%d ms)\n",
        actualTarget, audioPulseBytesToMs(actualTarget, bytesPerSecondValue),
        actualPrebuf, audioPulseBytesToMs(actualPrebuf, bytesPerSecondValue),
        actualMinRequest, audioPulseBytesToMs(actualMinRequest, bytesPerSecondValue),
        actualMax, audioPulseBytesToMs(actualMax, bytesPerSecondValue));
    logSink().debug("    audio output strategy: opened threaded server=`%s' rate=%d channels=%d format=%d bytes-per-second=%d target-bytes=%u prebuffer-bytes=%u min-request-bytes=%u max-bytes=%u target-ms=%d prebuffer-ms=%d min-request-ms=%d max-ms=%d configured-latency-ms=%d presentation-delay-samples=%d\n",
        serverDisplayName(), sampleSpec.rate,
        sampleSpec.channels, sampleSpec.format, bytesPerSecondValue, actualTarget,
        actualPrebuf, actualMinRequest, actualMax,
        audioPulseBytesToMs(actualTarget, bytesPerSecondValue),
        audioPulseBytesToMs(actualPrebuf, bytesPerSecondValue),
        audioPulseBytesToMs(actualMinRequest, bytesPerSecondValue),
        audioPulseBytesToMs(actualMax, bytesPerSecondValue),
        outputConfigValue.pulseLatencyMs, presentationDelaySamples);
}

int AudioPulseOutput::writeUnlocked(const void* buffer, int size, int waitForWritable) {
    if ((mainloop == NULL) || (stream == NULL) || (buffer == NULL) || (size <= 0))
        return 0;

    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;
    pa_stream* pulseStream = (pa_stream*)stream;
    int written = 0;
    int sampleBytes = bytesPerSample();
    if (sampleBytes <= 0)
        sampleBytes = 1;

    while (written < size) {
        if (!PA_STREAM_IS_GOOD(pa_stream_get_state(pulseStream))) {
            logSink().error("Pulse passthrough write failed on server `%s': stream is not ready\n",
                serverDisplayName());
            break;
        }

        size_t writable = pa_stream_writable_size(pulseStream);
        if (writable == (size_t)-1) {
            logSink().error("Pulse passthrough write failed on server `%s': %s\n",
                serverDisplayName(),
                pa_strerror(pa_context_errno((pa_context*)context)));
            break;
        }

        if (writable == 0) {
            if (waitForWritable)
                pa_threaded_mainloop_wait(pulseMainloop);
            else
                break;
            continue;
        }

        size_t chunk = (size_t)(size - written);
        if (chunk > writable)
            chunk = writable;
        if (sampleBytes > 1)
            chunk -= chunk % (size_t)sampleBytes;
        if (chunk == 0) {
            if (waitForWritable)
                pa_threaded_mainloop_wait(pulseMainloop);
            else
                break;
            continue;
        }

        if (pa_stream_write(pulseStream, (const char*)buffer + written, chunk,
                NULL, 0, PA_SEEK_RELATIVE)
            < 0) {
            logSink().error("Pulse passthrough write failed on server `%s': %s\n",
                serverDisplayName(),
                pa_strerror(pa_context_errno((pa_context*)context)));
            break;
        }

        written += (int)chunk;
    }

    return written;
}

int AudioPulseOutput::write(const void* buffer, int size) {
    if ((mainloop == NULL) || (stream == NULL) || (buffer == NULL) || (size <= 0))
        return 0;

    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;

    pa_threaded_mainloop_lock(pulseMainloop);
    int written = writeUnlocked(buffer, size, 1);
    pa_threaded_mainloop_unlock(pulseMainloop);

    return written;
}

int AudioPulseOutput::isOpen() const { return stream != NULL; }
int AudioPulseOutput::isRealtime() const { return 1; }

int AudioPulseOutput::adaptiveQueueBumpSamples() const {
    int sampleRate = samplesPerSecond();
    if (sampleRate <= 0)
        sampleRate = pcmFormatValue.sampleRate;
    int samples = sampleRate / 20;
    if (samples < 1)
        samples = 1;
    return samples;
}

int AudioPulseOutput::adaptiveQueueCapSamples() const {
    int sampleRate = samplesPerSecond();
    if (sampleRate <= 0)
        sampleRate = pcmFormatValue.sampleRate;
    int samples = sampleRate;
    if (samples < queuedTargetSamples())
        samples = queuedTargetSamples();
    return samples;
}

int AudioPulseOutput::queuedTargetSamples() const {
    int target = AudioOutput::queuedTargetSamples();
    int presentationTarget = pulsePresentationDelaySamples.load();
    int adaptiveTarget = adaptiveQueueTargetSamples.load();

    if (target < presentationTarget)
        target = presentationTarget;
    if (target < adaptiveTarget)
        target = adaptiveTarget;

    return target;
}

void AudioPulseOutput::growAdaptiveQueueTarget(int observedQueuedSamples,
    const char* reason) {
    int current = queuedTargetSamples();
    int next = current + adaptiveQueueBumpSamples();
    int cap = adaptiveQueueCapSamples();

    if (next > cap)
        next = cap;
    if (next <= current)
        return;

    adaptiveQueueTargetSamples.store(next);
    int sampleRate = samplesPerSecond();
    int nextMs = sampleRate > 0 ? (next * 1000) / sampleRate : 0;
    logSink().debug("audio runtime: pulse adaptive queue target increased reason=%s queued-samples=%d target-samples=%d target-ms=%d\n",
        reason ? reason : "unknown", observedQueuedSamples, next, nextMs);
}

void AudioPulseOutput::observeCallbackQueue(int queuedSamples) {
    if (queuedSamples <= 0)
        return;

    int target = queuedTargetSamples();
    if ((target > 0) && (queuedSamples < (target / 2)))
        growAdaptiveQueueTarget(queuedSamples, "low-water");
}

int AudioPulseOutput::drainUnlocked(size_t requestedBytes) {
    if (!callbackDrainActive.load() || (callbackStream == NULL) || (callbackScratch == NULL)
        || (callbackScratchSamples <= 0) || (stream == NULL))
        return 0;

    pa_stream* pulseStream = (pa_stream*)stream;
    int bytesPerSampleValue = callbackStream->bytesPerSample();
    int writes = 0;

    if (bytesPerSampleValue <= 0)
        return 0;

    for (;;) {
        if (!callbackDrainActive.load() || (callbackStream == NULL))
            break;
        if (!PA_STREAM_IS_GOOD(pa_stream_get_state(pulseStream))) {
            logSink().error("Pulse passthrough stream failed on server `%s': stream is not ready\n",
                serverDisplayName());
            break;
        }

        int skippedSamples = callbackStream->resyncIfBehind();
        if (skippedSamples > 0) {
            logSink().warn("Pulse passthrough fell behind decoded history; skipped %d samples.\n",
                skippedSamples);
        }

        int queuedSamples = callbackStream->queuedForOutputSamples();
        if (queuedSamples <= 0)
            break;

        // A delayed write callback may request only minreq bytes even though
        // the stream has much more room. Fill the actual writable space so the
        // server buffer can catch up after stalls.
        size_t streamWritableBytes = pa_stream_writable_size(pulseStream);
        if (streamWritableBytes == (size_t)-1) {
            logSink().error("Pulse passthrough writable query failed on server `%s': %s\n",
                serverDisplayName(),
                pa_strerror(pa_context_errno((pa_context*)context)));
            break;
        }

        if (streamWritableBytes == 0)
            break;

        int writableSamples = (int)(streamWritableBytes / (size_t)bytesPerSampleValue);
        int samplesWanted = queuedSamples;
        if (samplesWanted > callbackScratchSamples)
            samplesWanted = callbackScratchSamples;
        if (samplesWanted > writableSamples)
            samplesWanted = writableSamples;
        if (samplesWanted <= 0)
            break;

        long long startSample = callbackStream->submittedEndPosition();
        int samples = callbackStream->peekForOutput(callbackScratch, samplesWanted);
        if (samples <= 0)
            break;

        int bytes = pcmBytesForSamples(samples, bytesPerSampleValue);
        int written = writeUnlocked(callbackScratch, bytes, 0);
        if (written <= 0)
            break;

        int committedSamples = callbackStream->commitOutputSamples(
            written / bytesPerSampleValue);
        int committedBytes = pcmBytesForSamples(committedSamples, bytesPerSampleValue);
        if (committedSamples <= 0)
            break;

        dumpSubmittedPcm(callbackStream->format(), callbackScratch,
            committedBytes);
        int queuedAfter = callbackStream->queuedForOutputSamples();
        observeCallbackQueue(queuedAfter);
        reportSubmittedPcm(callbackStream->format(), callbackScratch,
            committedSamples, committedBytes, written,
            queuedAfter,
            callbackStream->submittedEndPosition());
        logSink().trace("audio runtime",
            "pulse callback drained samples=%d bytes=%d written=%d committed-samples=%d committed-bytes=%d writable-bytes=%lu requested-bytes=%lu queued-samples=%d submitted-start-sample=%lld submitted-end-sample=%lld input-finished=%d\n",
            samples, bytes, written, committedSamples, committedBytes,
            (unsigned long)streamWritableBytes,
            (unsigned long)requestedBytes, queuedAfter,
            startSample, callbackStream->submittedEndPosition(),
            callbackInputFinished ? callbackInputFinished->load() : 0);

        writes++;
        if (written < bytes)
            break;
    }

    return writes;
}

int AudioPulseOutput::supportsCallbackDrain() const {
    return (mainloop != NULL) && (stream != NULL) && (drainEvent != NULL);
}

void AudioPulseOutput::startCallbackDrain(AudioOutputStream& outputStream,
    const std::atomic<int>* inputFinished, int scratchSamples) {
    if ((mainloop == NULL) || (stream == NULL) || (drainEvent == NULL))
        return;

    int samples = scratchSamples;
    if (samples <= 0)
        samples = targetDelaySamples();
    if (samples <= 0)
        samples = 1024;

    char* scratch = new char[pcmBytesForSamples(samples, outputStream.bytesPerSample())];
    char* oldScratch = NULL;
    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;

    pa_threaded_mainloop_lock(pulseMainloop);
    oldScratch = callbackScratch;
    callbackStream = &outputStream;
    callbackInputFinished = inputFinished;
    callbackScratch = scratch;
    callbackScratchSamples = samples;
    callbackDrainActive.store(1);
    pa_mainloop_api* api = pa_threaded_mainloop_get_api(pulseMainloop);
    if ((api != NULL) && (drainEvent != NULL))
        api->defer_enable((pa_defer_event*)drainEvent, 1);
    pa_threaded_mainloop_unlock(pulseMainloop);

    delete[] oldScratch;

    logSink().debug("audio runtime: pulse callback drain started scratch-samples=%d target-buffer-samples=%d queued-samples=%d input-finished=%d\n",
        samples, queuedTargetSamples(), outputStream.queuedForOutputSamples(),
        inputFinished ? inputFinished->load() : 0);
}

void AudioPulseOutput::notifyCallbackDrain() {
    if ((mainloop == NULL) || (drainEvent == NULL))
        return;

    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;
    pa_threaded_mainloop_lock(pulseMainloop);
    if (callbackDrainActive.load()) {
        pa_mainloop_api* api = pa_threaded_mainloop_get_api(pulseMainloop);
        if (api != NULL)
            api->defer_enable((pa_defer_event*)drainEvent, 1);
    }
    pa_threaded_mainloop_unlock(pulseMainloop);
}

void AudioPulseOutput::stopCallbackDrain() {
    if (mainloop == NULL) {
        delete[] callbackScratch;
        callbackScratch = NULL;
        callbackStream = NULL;
        callbackInputFinished = NULL;
        callbackScratchSamples = 0;
        callbackDrainActive.store(0);
        return;
    }

    char* oldScratch = NULL;
    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;

    pa_threaded_mainloop_lock(pulseMainloop);
    callbackDrainActive.store(0);
    callbackStream = NULL;
    callbackInputFinished = NULL;
    callbackScratchSamples = 0;
    oldScratch = callbackScratch;
    callbackScratch = NULL;
    if (drainEvent != NULL) {
        pa_mainloop_api* api = pa_threaded_mainloop_get_api(pulseMainloop);
        if (api != NULL)
            api->defer_enable((pa_defer_event*)drainEvent, 0);
    }
    pa_threaded_mainloop_unlock(pulseMainloop);

    delete[] oldScratch;
}

int AudioPulseOutput::callbackDrainComplete(const AudioOutputStream& stream,
    int inputFinished) const {
    return callbackDrainActive.load() && playbackComplete(stream, inputFinished);
}

void AudioPulseOutput::pulseWritable(size_t requestedBytes) {
    drainUnlocked(requestedBytes);
    if (mainloop != NULL)
        pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);
}

void AudioPulseOutput::pulseUnderflow() {
    int underflows = underflowCountValue.fetch_add(1) + 1;
    int previous = lastReportedUnderflows.exchange(underflows);

    growAdaptiveQueueTarget(0, "underflow");
    drainUnlocked((size_t)-1);
    if (mainloop != NULL)
        pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);

    if (underflows != previous)
        logSink().warn("Pulse output underrun on server `%s' (count=%d).\n",
            serverDisplayName(), underflows);
}

const char* AudioPulseOutput::serverName() const {
    return outputConfigValue.pulseServerName();
}

const char* AudioPulseOutput::serverDisplayName() const {
    return outputConfigValue.pulseServerDisplayName();
}

int AudioPulseOutput::pulseLatencyMs() const {
    return outputConfigValue.pulseLatencyMs;
}

int AudioPulseOutput::presentationDelaySamples() const {
    int samples = pulsePresentationDelaySamples.load();
    return samples > 0 ? samples : AudioOutput::presentationDelaySamples();
}

int AudioPulseOutput::service(AudioOutputStream& outputStream, char* scratch, int scratchSamples,
    int /*inputFinished*/) {
    if (callbackDrainActive.load())
        return 0;

    if ((mainloop == NULL) || (stream == NULL) || (scratch == NULL)
        || (scratchSamples <= 0))
        return 0;

    int skippedSamples = outputStream.resyncIfBehind();
    if (skippedSamples > 0) {
        logSink().warn("Pulse passthrough fell behind decoded history; skipped %d samples.\n",
            skippedSamples);
    }

    int queuedSamples = outputStream.queuedForOutputSamples();
    if (queuedSamples <= 0)
        return 0;

    int bytesPerSampleValue = outputStream.bytesPerSample();
    if (bytesPerSampleValue <= 0)
        return 0;

    pa_threaded_mainloop* pulseMainloop = (pa_threaded_mainloop*)mainloop;
    pa_stream* pulseStream = (pa_stream*)stream;
    size_t writableBytes = 0;
    int underflows = 0;
    int streamGood = 1;

    pa_threaded_mainloop_lock(pulseMainloop);
    streamGood = PA_STREAM_IS_GOOD(pa_stream_get_state(pulseStream));
    if (streamGood) {
        writableBytes = pa_stream_writable_size(pulseStream);
        if (writableBytes == (size_t)-1)
            streamGood = 0;
    }
    underflows = underflowCountValue.load();
    pa_threaded_mainloop_unlock(pulseMainloop);

    if (!streamGood) {
        logSink().error("Pulse passthrough stream failed on server `%s': %s\n",
            serverDisplayName(),
            pa_strerror(pa_context_errno((pa_context*)context)));
        return 0;
    }

    if (underflows != lastReportedUnderflows.load()) {
        logSink().warn("Pulse output underrun on server `%s' (count=%d).\n",
            serverDisplayName(), underflows);
        lastReportedUnderflows.store(underflows);
    }

    if (writableBytes == 0) {
        logSink().trace("audio runtime",
            "pulse output idle writable-bytes=0 queued-samples=%d submitted-end-sample=%lld\n",
            queuedSamples, outputStream.submittedEndPosition());
        return 0;
    }

    int writableSamples = (int)(writableBytes / (size_t)bytesPerSampleValue);
    if (writableSamples <= 0)
        return 0;

    int samplesWanted = (queuedSamples < scratchSamples) ? queuedSamples : scratchSamples;
    if (samplesWanted > writableSamples)
        samplesWanted = writableSamples;
    if (samplesWanted <= 0)
        return 0;

    long long startSample = outputStream.submittedEndPosition();
    int samples = outputStream.peekForOutput(scratch, samplesWanted);
    if (samples <= 0)
        return 0;

    int bytes = pcmBytesForSamples(samples, bytesPerSampleValue);
    int written = write(scratch, bytes);
    int committedSamples = 0;
    int committedBytes = 0;
    if (written > 0) {
        committedSamples = outputStream.commitOutputSamples(written / bytesPerSampleValue);
        committedBytes = pcmBytesForSamples(committedSamples, bytesPerSampleValue);
    }
    if (committedSamples > 0) {
        dumpSubmittedPcm(outputStream.format(), scratch, committedBytes);
    }

    reportSubmittedPcm(outputStream.format(), scratch, committedSamples,
        committedBytes, written, outputStream.queuedForOutputSamples(),
        outputStream.submittedEndPosition());
    logSink().trace("audio runtime",
        "pulse output submitted samples=%d bytes=%d written=%d committed-samples=%d committed-bytes=%d writable-bytes=%lu queued-samples=%d submitted-start-sample=%lld submitted-end-sample=%lld\n",
        samples, bytes, written, committedSamples, committedBytes,
        (unsigned long)writableBytes,
        outputStream.queuedForOutputSamples(), startSample, outputStream.submittedEndPosition());

    return committedSamples > 0 ? 1 : 0;
}

#else

AudioPulseOutput::AudioPulseOutput(const PcmFormat& format, SecondsClock& clock_,
    LogSink& log_,
    const AudioOutputConfig& config, AudioOutputDump* outputDump, int)
    : AudioOutput(config.pulseOutputTargetLatencyMs, outputDump, clock_, log_)
    , pcmFormatValue(format)
    , outputConfigValue(config)
    , mainloop(NULL)
    , context(NULL)
    , stream(NULL)
    , drainEvent(NULL)
    , callbackStream(NULL)
    , callbackInputFinished(NULL)
    , callbackScratch(NULL)
    , callbackScratchSamples(0)
    , callbackDrainActive(0)
    , bytesPerSecondValue(0)
    , pulsePresentationDelaySamples(0)
    , adaptiveQueueTargetSamples(0)
    , underflowCountValue(0)
    , lastReportedUnderflows(0) {
    logSink().debug("    audio output strategy: Pulse unavailable because support is not compiled in\n");
}

AudioPulseOutput::~AudioPulseOutput() { }
void AudioPulseOutput::closePulse() { }
int AudioPulseOutput::defaultTargetLatencyMs() const { return AudioOutput::defaultTargetLatencyMs(); }
int AudioPulseOutput::timingScratchSamples(int, int targetDelaySamples) const { return targetDelaySamples; }
int AudioPulseOutput::write(const void*, int) { return 0; }
int AudioPulseOutput::isOpen() const { return 0; }
int AudioPulseOutput::isRealtime() const { return 1; }
void AudioPulseOutput::update() { }
int AudioPulseOutput::service(AudioOutputStream&, char*, int, int) { return 0; }
int AudioPulseOutput::supportsCallbackDrain() const { return 0; }
void AudioPulseOutput::startCallbackDrain(AudioOutputStream&, const std::atomic<int>*, int) { }
void AudioPulseOutput::notifyCallbackDrain() { }
void AudioPulseOutput::stopCallbackDrain() { }
int AudioPulseOutput::callbackDrainComplete(const AudioOutputStream& stream,
    int inputFinished) const { return playbackComplete(stream, inputFinished); }
void AudioPulseOutput::pulseWritable(size_t) { }
void AudioPulseOutput::pulseUnderflow() { underflowCountValue.fetch_add(1); }
const char* AudioPulseOutput::serverName() const { return outputConfigValue.pulseServerName(); }
const char* AudioPulseOutput::serverDisplayName() const { return outputConfigValue.pulseServerDisplayName(); }
int AudioPulseOutput::pulseLatencyMs() const { return outputConfigValue.pulseLatencyMs; }
int AudioPulseOutput::presentationDelaySamples() const { return AudioOutput::presentationDelaySamples(); }
int AudioPulseOutput::queuedTargetSamples() const { return AudioOutput::queuedTargetSamples(); }

#endif
