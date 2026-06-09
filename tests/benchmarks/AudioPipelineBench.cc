#include "Audio.h"
#include "AudioAnalyzer.h"
#include "AudioFftProcessor.h"
#include "AudioFrame.h"
#include "AudioFramePipeline.h"
#include "AudioProcessing.h"
#include "AudioProcessor.h"
#include "AudioTypes.h"
#include "ProcessServices.h"
#include "configuration_defaults.h"

#include <benchmark/benchmark.h>

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const char* audioSampleFormatText(int) { return "benchmark-format"; }

namespace {

const int kVisualFrameSamples = 1024;
const int kAudioSliceMs = 10;
const int kProtectedHistorySamples = 44100;
const int kBufferCapacitySamples = 44100 * 3;

class DeterministicRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) {
        return 0;
    }
};

class DeterministicSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

class NullLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

int samplesForAudioSlice(const PcmFormat& format) {
    return (format.sampleRate * kAudioSliceMs) / 1000;
}

std::string fixturePath(const char* fileName) {
    return std::string(CTH_AUDIO_FIXTURE_DIR) + "/" + fileName;
}

const char* primaryFixturePath() {
    static std::string path = fixturePath("sine-100-1600-doubling-10s.wav");
    return path.c_str();
}

const char* prismPcmFixturePath() {
    static std::string path = fixturePath("prism-2s-48000-stereo-s16le.raw");
    return path.c_str();
}

struct PcmFixture {
    PcmFormat format;
    int sliceSamples;
    std::vector<char> frame1024;
    std::vector<char> slice10ms;
    std::vector<char> threeSeconds;

    PcmFixture()
        : sliceSamples(0) {
        NullLogSink log;
        WavPcmSource source(primaryFixturePath(), log);
        if (source.hasError())
            return;

        format = source.format();
        sliceSamples = samplesForAudioSlice(format);
        frame1024.resize(format.bytesForSamples(kVisualFrameSamples));
        source.read(frame1024.data(), (int)frame1024.size(), kVisualFrameSamples);

        slice10ms.resize(format.bytesForSamples(sliceSamples));
        source.read(slice10ms.data(), (int)slice10ms.size(), sliceSamples);

        threeSeconds.resize(format.bytesForSamples(kBufferCapacitySamples));
        int samplesRead = source.read(threeSeconds.data(), (int)threeSeconds.size(),
            kBufferCapacitySamples);
        threeSeconds.resize(format.bytesForSamples(samplesRead));
    }

    int bytesPerSample() const { return format.bytesPerSample(); }
    int threeSecondSamples() const {
        return bytesPerSample() > 0 ? (int)threeSeconds.size() / bytesPerSample() : 0;
    }
};

const PcmFixture& pcmFixture() {
    static PcmFixture fixture;
    return fixture;
}

struct PrismPcmFrameFixture {
    int valid;
    std::string error;
    PcmFormat format;
    AudioFrame frame;

    PrismPcmFrameFixture()
        : valid(0)
        , error()
        , format()
        , frame() {
        FILE* file = fopen(prismPcmFixturePath(), "rb");
        if (!file) {
            error = std::string("could not open prism PCM fixture: ")
                + prismPcmFixturePath();
            return;
        }

        format.sampleRate = 48000;
        format.channels = 2;
        format.sampleFormat = SF_s16_le;

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            error = std::string("could not size prism PCM fixture: ")
                + prismPcmFixturePath();
            return;
        }
        long byteCount = ftell(file);
        if (byteCount < 0) {
            fclose(file);
            error = std::string("could not tell prism PCM fixture size: ")
                + prismPcmFixturePath();
            return;
        }
        rewind(file);

        int bytesPerSample = format.bytesPerSample();
        if ((byteCount <= 0) || ((byteCount % bytesPerSample) != 0)) {
            fclose(file);
            error = std::string("prism PCM fixture has unexpected byte count: ")
                + prismPcmFixturePath();
            return;
        }

        std::vector<char> pcm((size_t)byteCount);
        size_t bytesRead = fread(pcm.data(), 1, pcm.size(), file);
        fclose(file);
        if (bytesRead != pcm.size()) {
            error = std::string("short read from prism PCM fixture: ")
                + prismPcmFixturePath();
            return;
        }

        int samplesRead = (int)pcm.size() / bytesPerSample;
        if (samplesRead < kVisualFrameSamples) {
            error = std::string("prism PCM fixture too short for one visual frame: ")
                + prismPcmFixturePath();
            return;
        }

        NullLogSink log;
        DecodedAudioHistory history(samplesRead, format,
            kVisualFrameSamples, log);
        AudioFrameBuilder builder(log);
        history.appendDecodedPcm(pcm.data(), samplesRead);
        frame.clear();
        builder.build(frame, history, samplesRead / 2);
        valid = 1;
    }
};

const PrismPcmFrameFixture& prismPcmFrameFixture() {
    static PrismPcmFrameFixture fixture;
    return fixture;
}

static int requirePrismPcmFrameFixture(benchmark::State& state) {
    const PrismPcmFrameFixture& fixture = prismPcmFrameFixture();
    if (!fixture.valid) {
        state.SkipWithError(fixture.error.c_str());
        return 0;
    }
    return 1;
}

void fillFrameFromFixture(AudioFrame& frame) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    DecodedAudioHistory history(kVisualFrameSamples * 2, fixture.format,
        kVisualFrameSamples, log);
    AudioFrameBuilder builder(log);

    frame.clear();
    history.appendDecodedPcm(fixture.frame1024.data(), kVisualFrameSamples);
    builder.build(frame, history, kVisualFrameSamples / 2);
}

void primeHistory(DecodedAudioHistory& history) {
    const PcmFixture& fixture = pcmFixture();
    history.clear();
    history.appendDecodedPcm(fixture.threeSeconds.data(), fixture.threeSecondSamples());
}

void refillHistoryIfNeeded(DecodedAudioHistory& history, AudioOutputStream& stream) {
    const PcmFixture& fixture = pcmFixture();
    if (stream.queuedForOutputSamples() < fixture.sliceSamples * 2) {
        history.clear();
        history.appendDecodedPcm(fixture.threeSeconds.data(), fixture.threeSecondSamples());
        stream.reset(0);
    }
}

} // namespace

static void BM_Wav_OpenParse(benchmark::State& state) {
    NullLogSink log;
    for (auto _ : state) {
        WavPcmSource source(primaryFixturePath(), log);
        int sampleRate = source.format().sampleRate;
        benchmark::DoNotOptimize(source.hasError());
        benchmark::DoNotOptimize(sampleRate);
    }
}

static void BM_Wav_Read1024(benchmark::State& state) {
    NullLogSink log;
    WavPcmSource source(primaryFixturePath(), log);
    std::vector<char> scratch(source.format().bytesForSamples(kVisualFrameSamples));
    long long totalSamples = 0;

    for (auto _ : state) {
        int samples = source.read(scratch.data(), (int)scratch.size(), kVisualFrameSamples);
        if (samples <= 0) {
            state.PauseTiming();
            source.rewind();
            state.ResumeTiming();
            samples = source.read(scratch.data(), (int)scratch.size(), kVisualFrameSamples);
        }
        totalSamples += samples;
        benchmark::DoNotOptimize(samples);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_Wav_Read10ms(benchmark::State& state) {
    NullLogSink log;
    WavPcmSource source(primaryFixturePath(), log);
    int sliceSamples = samplesForAudioSlice(source.format());
    std::vector<char> scratch(source.format().bytesForSamples(sliceSamples));
    long long totalSamples = 0;

    for (auto _ : state) {
        int samples = source.read(scratch.data(), (int)scratch.size(), sliceSamples);
        if (samples <= 0) {
            state.PauseTiming();
            source.rewind();
            state.ResumeTiming();
            samples = source.read(scratch.data(), (int)scratch.size(), sliceSamples);
        }
        totalSamples += samples;
        benchmark::DoNotOptimize(samples);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioInput_Read10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    AudioInput input(new WavPcmSource(primaryFixturePath(), log), log, 1, 1);
    int bytesPerSample = fixture.bytesPerSample();
    int sliceSamples = samplesForAudioSlice(fixture.format);
    std::vector<char> scratch(pcmBytesForSamples(sliceSamples, bytesPerSample));
    long long totalSamples = 0;

    for (auto _ : state) {
        int samples = input.read(scratch.data(), (int)scratch.size(), sliceSamples);
        if (samples <= 0) {
            state.PauseTiming();
            input.update();
            state.ResumeTiming();
            samples = input.read(scratch.data(), (int)scratch.size(), sliceSamples);
        }
        totalSamples += samples;
        benchmark::DoNotOptimize(samples);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_DecodedAudioHistory_Append10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.format,
        kProtectedHistorySamples, log);
    long long totalSamples = 0;

    for (auto _ : state) {
        if (history.writableSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            history.clear();
            state.ResumeTiming();
        }
        int appended = history.appendDecodedPcm(fixture.slice10ms.data(), fixture.sliceSamples);
        totalSamples += appended;
        benchmark::DoNotOptimize(appended);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioOutput_NullService10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.format,
        kProtectedHistorySamples, log);
    AudioOutputStream stream(history);
    DeterministicSecondsClock clock;
    AudioNullOutput output(clock, log);
    std::vector<char> scratch(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    primeHistory(history);
    output.configureTiming(fixture.format.sampleRate, fixture.bytesPerSample(),
        fixture.sliceSamples);

    for (auto _ : state) {
        if (stream.queuedForOutputSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            refillHistoryIfNeeded(history, stream);
            state.ResumeTiming();
        }
        int queuedBefore = stream.queuedForOutputSamples();
        int writes = output.service(stream, scratch.data(), fixture.sliceSamples, 0);
        totalSamples += queuedBefore - stream.queuedForOutputSamples();
        benchmark::DoNotOptimize(writes);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioOutputStream_PeekCommit10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.format,
        kProtectedHistorySamples, log);
    AudioOutputStream stream(history);
    std::vector<char> scratch(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    primeHistory(history);

    for (auto _ : state) {
        if (stream.queuedForOutputSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            primeHistory(history);
            stream.reset(0);
            state.ResumeTiming();
        }

        int samples = stream.peekForOutput(scratch.data(), fixture.sliceSamples);
        int committed = stream.commitOutputSamples(samples);
        totalSamples += committed;
        benchmark::DoNotOptimize(samples);
        benchmark::DoNotOptimize(committed);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioFrameBuilder_Build1024(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink log;
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.format,
        kProtectedHistorySamples, log);
    AudioFrameBuilder builder(log);
    AudioFrame frame;
    long long centerSample = kVisualFrameSamples;
    long long totalSamples = 0;

    primeHistory(history);

    for (auto _ : state) {
        builder.build(frame, history, centerSample);
        centerSample += kVisualFrameSamples;
        if (centerSample + kVisualFrameSamples >= fixture.threeSecondSamples()) {
            centerSample = kVisualFrameSamples;
        }
        totalSamples += frame.samples;
        benchmark::DoNotOptimize(frame.samples);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioProcessor_None(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        processor.none(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::ClobberMemory();
    }
}

static void BM_AudioProcessor_Filter1(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        processor.filter1(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::ClobberMemory();
    }
}

static void BM_AudioProcessor_Filter2(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        processor.filter2(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::ClobberMemory();
    }
}

static void BM_AudioProcessor_FFT(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        processor.fft(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::ClobberMemory();
    }
}

static void BM_AudioFft_FixedPoint_PrismPcm(benchmark::State& state) {
    if (!requirePrismPcmFrameFixture(state))
        return;

    const PrismPcmFrameFixture& fixture = prismPcmFrameFixture();
    FixedPointAudioFftProcessor processor;
    AudioFrame frame = fixture.frame;

    for (auto _ : state) {
        processor.transform(frame.raw, frame.processedWaveData);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::ClobberMemory();
    }
}

static void BM_AudioProcessor_Analyze1024(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        AudioMetrics metrics = processor.analyze(frame.raw,
            AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE);
        benchmark::DoNotOptimize(metrics.amplitude);
    }
}

static void BM_AcousticContext_Update(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    AcousticContext acousticContext;
    fillFrameFromFixture(frame);
    AudioMetrics metrics = processor.analyze(frame.raw,
        AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE);

    for (auto _ : state) {
        acousticContext.update(metrics);
        benchmark::DoNotOptimize(acousticContext.intensity());
    }
}

static void BM_AudioFramePipeline_ProcessFrameNone(benchmark::State& state) {
    AudioFrame frame;
    AudioProcessor processor;
    DeterministicRandomSource randomSource;
    DeterministicSecondsClock clock;
    NullLogSink log;
    AcousticContext acousticContext(&log);
    AudioProcessingState processingState(randomSource);
    AudioProcessingSelector processingSelector(processingState, processor, log);
    fillFrameFromFixture(frame);
    DefaultAudioFramePipeline pipeline(acousticContext, processingSelector,
        processor, clock, log, AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE);

    processingSelector.changeTo("none");

    for (auto _ : state) {
        pipeline.processFrame(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(frame.metrics.amplitude);
        benchmark::ClobberMemory();
    }
}

static void BM_EndToEnd_Process10msWavToNullOutputToPipelineNone(
    benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    NullLogSink inputLog;
    AudioInput input(new WavPcmSource(primaryFixturePath(), inputLog),
        inputLog, 1, 1);
    NullLogSink log;
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.format,
        kProtectedHistorySamples, log);
    AudioOutputStream stream(history);
    DeterministicSecondsClock outputClock;
    NullLogSink outputLog;
    AudioNullOutput output(outputClock, outputLog);
    AudioFrameBuilder builder(log);
    AudioFrame frame;
    AcousticContext acousticContext(&log);
    AudioProcessor processor;
    DeterministicRandomSource randomSource;
    DeterministicSecondsClock clock;
    AudioProcessingState processingState(randomSource);
    AudioProcessingSelector processingSelector(processingState, processor, log);
    DefaultAudioFramePipeline pipeline(acousticContext, processingSelector,
        processor, clock, log, AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE);
    std::vector<char> inputChunk(fixture.format.bytesForSamples(fixture.sliceSamples));
    std::vector<char> outputChunk(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    processingSelector.changeTo("none");
    output.configureTiming(fixture.format.sampleRate, fixture.bytesPerSample(),
        fixture.sliceSamples);

    for (auto _ : state) {
        int samples = input.read(inputChunk.data(), (int)inputChunk.size(),
            fixture.sliceSamples);
        if (samples <= 0) {
            state.PauseTiming();
            input.update();
            history.clear();
            stream.reset(0);
            state.ResumeTiming();
            samples = input.read(inputChunk.data(), (int)inputChunk.size(),
                fixture.sliceSamples);
        }

        history.appendDecodedPcm(inputChunk.data(), samples);
        output.service(stream, outputChunk.data(), fixture.sliceSamples, 0);
        builder.build(frame, history, stream.submittedEndPosition());
        pipeline.processFrame(frame);

        totalSamples += samples;
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(frame.metrics.amplitude);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

BENCHMARK(BM_Wav_OpenParse);
BENCHMARK(BM_Wav_Read1024);
BENCHMARK(BM_Wav_Read10ms);
BENCHMARK(BM_AudioInput_Read10ms);
BENCHMARK(BM_DecodedAudioHistory_Append10ms);
BENCHMARK(BM_AudioOutput_NullService10ms);
BENCHMARK(BM_AudioOutputStream_PeekCommit10ms);
BENCHMARK(BM_AudioFrameBuilder_Build1024);
BENCHMARK(BM_AudioProcessor_None);
BENCHMARK(BM_AudioProcessor_Filter1);
BENCHMARK(BM_AudioProcessor_Filter2);
BENCHMARK(BM_AudioProcessor_FFT);
BENCHMARK(BM_AudioFft_FixedPoint_PrismPcm);
BENCHMARK(BM_AudioProcessor_Analyze1024);
BENCHMARK(BM_AcousticContext_Update);
BENCHMARK(BM_AudioFramePipeline_ProcessFrameNone);
BENCHMARK(BM_EndToEnd_Process10msWavToNullOutputToPipelineNone);
