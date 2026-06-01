#include "Audio.h"
#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"
#include "AudioTypes.h"
#include "AudioVisualBridge.h"
#include "Option.h"

#include <benchmark/benchmark.h>

#include <limits.h>
#include <stddef.h>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static OptionInt audioInputModeImpl("sound-device-number", AIM_File, AIM_Max);
static OptionInt soundFormatImpl("sound-format", SF_s16_le);
static OptionInt soundChannelsImpl("sound-channels", 2);
static OptionInt soundSampleRateImpl("sound-sample-rate", 44100);
static OptionInt soundDSPMethodImpl("sound-method", 0);
static OptionInt soundDSPFragmentsImpl("sound-fragments", 16);
static OptionInt soundDSPFragmentSizeImpl("sound-fragment-size", 0);
static OptionOnOff soundDSPSyncImpl("sound-sync", 0);
static OptionOnOff soundSilentImpl("silent", 0);

Option& audioInputMode = audioInputModeImpl;
Option& soundFormat = soundFormatImpl;
Option& soundChannels = soundChannelsImpl;
Option& soundSampleRate = soundSampleRateImpl;
Option& soundDSPMethod = soundDSPMethodImpl;
Option& soundDSPFragments = soundDSPFragmentsImpl;
Option& soundDSPFragmentSize = soundDSPFragmentSizeImpl;
Option& soundDSPSync = soundDSPSyncImpl;
Option& soundSilent = soundSilentImpl;

int audioInputLoop = 0;
char dev_dsp[PATH_MAX] = "";

int init_mixer() {
    return 0;
}

namespace {

const int kVideoFrameSamples = 1024;
const int kAudioSliceMs = 10;
const int kProtectedHistorySamples = 44100;
const int kBufferCapacitySamples = 44100 * 3;

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

void applyFormat(const PcmFormat& format) {
    soundSampleRate.setValue(format.sampleRate);
    soundChannels.setValue(format.channels);
    soundFormat.setValue(format.sampleFormat);
}

struct PcmFixture {
    PcmFormat format;
    int sliceSamples;
    std::vector<char> frame1024;
    std::vector<char> slice10ms;
    std::vector<char> threeSeconds;

    PcmFixture()
        : sliceSamples(0) {
        WavPcmSource source(primaryFixturePath());
        if (source.hasError())
            return;

        format = source.format();
        sliceSamples = samplesForAudioSlice(format);
        applyFormat(format);

        frame1024.resize(format.bytesForSamples(kVideoFrameSamples));
        source.read(frame1024.data(), (int)frame1024.size(), kVideoFrameSamples);

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

void fillFrameFromFixture(AudioFrame& frame) {
    const PcmFixture& fixture = pcmFixture();
    AudioBuffer buffer(kVideoFrameSamples * 2, fixture.bytesPerSample(), kVideoFrameSamples);
    AudioFrameBuilder builder;

    applyFormat(fixture.format);
    frame.clear();
    buffer.appendDecodedPcm(fixture.frame1024.data(), kVideoFrameSamples);
    builder.build(frame, buffer, kVideoFrameSamples / 2);
}

void primeBuffer(AudioBuffer& buffer) {
    const PcmFixture& fixture = pcmFixture();
    buffer.clear();
    buffer.appendDecodedPcm(fixture.threeSeconds.data(), fixture.threeSecondSamples());
}

void refillBufferIfNeeded(AudioBuffer& buffer) {
    const PcmFixture& fixture = pcmFixture();
    if (buffer.queuedForOutputSamples() < fixture.sliceSamples * 2) {
        buffer.clear();
        buffer.appendDecodedPcm(fixture.threeSeconds.data(), fixture.threeSecondSamples());
    }
}

} // namespace

static void BM_Wav_OpenParse(benchmark::State& state) {
    for (auto _ : state) {
        WavPcmSource source(primaryFixturePath());
        int sampleRate = source.format().sampleRate;
        benchmark::DoNotOptimize(source.hasError());
        benchmark::DoNotOptimize(sampleRate);
    }
}

static void BM_Wav_Read1024(benchmark::State& state) {
    WavPcmSource source(primaryFixturePath());
    std::vector<char> scratch(source.format().bytesForSamples(kVideoFrameSamples));
    long long totalSamples = 0;

    for (auto _ : state) {
        int samples = source.read(scratch.data(), (int)scratch.size(), kVideoFrameSamples);
        if (samples <= 0) {
            state.PauseTiming();
            source.rewind();
            state.ResumeTiming();
            samples = source.read(scratch.data(), (int)scratch.size(), kVideoFrameSamples);
        }
        totalSamples += samples;
        benchmark::DoNotOptimize(samples);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_Wav_Read10ms(benchmark::State& state) {
    WavPcmSource source(primaryFixturePath());
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
    int previousLoop = audioInputLoop;
    audioInputLoop = 1;
    AudioInput input(new WavPcmSource(primaryFixturePath()));
    int bytesPerSample = (soundFormat < 2) ? int(soundChannels) : 2 * int(soundChannels);
    int sliceSamples = (int(soundSampleRate) * kAudioSliceMs) / 1000;
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
    audioInputLoop = previousLoop;
}

static void BM_AudioBuffer_Append10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    AudioBuffer buffer(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    long long totalSamples = 0;

    for (auto _ : state) {
        if (buffer.writableSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            buffer.clear();
            state.ResumeTiming();
        }
        int appended = buffer.appendDecodedPcm(fixture.slice10ms.data(), fixture.sliceSamples);
        totalSamples += appended;
        benchmark::DoNotOptimize(appended);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioOutput_NullService10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    AudioBuffer buffer(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioNullOutput output;
    std::vector<char> scratch(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    primeBuffer(buffer);
    output.configureTiming(fixture.format.sampleRate, fixture.bytesPerSample(),
        fixture.sliceSamples);

    for (auto _ : state) {
        if (buffer.queuedForOutputSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            refillBufferIfNeeded(buffer);
            state.ResumeTiming();
        }
        int queuedBefore = buffer.queuedForOutputSamples();
        int writes = output.service(buffer, scratch.data(), fixture.sliceSamples, 0);
        totalSamples += queuedBefore - buffer.queuedForOutputSamples();
        benchmark::DoNotOptimize(writes);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioBuffer_PeekCommit10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    AudioBuffer buffer(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    std::vector<char> scratch(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    primeBuffer(buffer);

    for (auto _ : state) {
        if (buffer.queuedForOutputSamples() < fixture.sliceSamples) {
            state.PauseTiming();
            primeBuffer(buffer);
            state.ResumeTiming();
        }

        int samples = buffer.peekForOutput(scratch.data(), fixture.sliceSamples);
        int committed = buffer.commitOutputSamples(samples);
        totalSamples += committed;
        benchmark::DoNotOptimize(samples);
        benchmark::DoNotOptimize(committed);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(totalSamples);
}

static void BM_AudioFrameBuilder_Build1024(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    AudioBuffer buffer(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioFrameBuilder builder;
    AudioFrame frame;
    long long centerSample = kVideoFrameSamples;
    long long totalSamples = 0;

    applyFormat(fixture.format);
    primeBuffer(buffer);

    for (auto _ : state) {
        builder.build(frame, buffer, centerSample);
        centerSample += kVideoFrameSamples;
        if (centerSample + kVideoFrameSamples >= fixture.threeSecondSamples()) {
            centerSample = kVideoFrameSamples;
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

static void BM_AudioAnalyzer_Analyze1024(benchmark::State& state) {
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        AudioMetrics metrics = audioAnalyzer.analyze(frame.raw);
        benchmark::DoNotOptimize(metrics.amplitude);
    }
}

static void BM_AcousticContext_Update(benchmark::State& state) {
    AudioFrame frame;
    fillFrameFromFixture(frame);
    AudioMetrics metrics = audioAnalyzer.analyze(frame.raw);

    for (auto _ : state) {
        acousticContext.update(metrics);
        benchmark::DoNotOptimize(acousticContext.intensity());
    }
}

static void BM_AudioVisualBridge_RunFrameNone(benchmark::State& state) {
    AudioFrame frame;
    fillFrameFromFixture(frame);
    AudioVisualBridge bridge;

    audioProcessing.change("none");
    audioFrameSetTestOverride(&frame);

    for (auto _ : state) {
        bridge.runFrame();
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(audioMetrics.amplitude);
        benchmark::ClobberMemory();
    }

    audioFrameSetTestOverride(NULL);
}

static void BM_EndToEnd_Process10msWavToNullOutputToBridgeNone(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    int previousLoop = audioInputLoop;
    audioInputLoop = 1;
    AudioInput input(new WavPcmSource(primaryFixturePath()));
    AudioBuffer buffer(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioNullOutput output;
    AudioFrameBuilder builder;
    AudioFrame frame;
    AudioVisualBridge bridge;
    std::vector<char> inputChunk(fixture.format.bytesForSamples(fixture.sliceSamples));
    std::vector<char> outputChunk(fixture.format.bytesForSamples(fixture.sliceSamples));
    long long totalSamples = 0;

    audioProcessing.change("none");
    audioFrameSetTestOverride(&frame);
    output.configureTiming(fixture.format.sampleRate, fixture.bytesPerSample(),
        fixture.sliceSamples);

    for (auto _ : state) {
        int samples = input.read(inputChunk.data(), (int)inputChunk.size(),
            fixture.sliceSamples);
        if (samples <= 0) {
            state.PauseTiming();
            input.update();
            buffer.clear();
            state.ResumeTiming();
            samples = input.read(inputChunk.data(), (int)inputChunk.size(),
                fixture.sliceSamples);
        }

        buffer.appendDecodedPcm(inputChunk.data(), samples);
        output.service(buffer, outputChunk.data(), fixture.sliceSamples, 0);
        builder.build(frame, buffer, buffer.submittedEndPosition());
        bridge.runFrame();

        totalSamples += samples;
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(audioMetrics.amplitude);
        benchmark::ClobberMemory();
    }

    audioFrameSetTestOverride(NULL);
    state.SetItemsProcessed(totalSamples);
    audioInputLoop = previousLoop;
}

BENCHMARK(BM_Wav_OpenParse);
BENCHMARK(BM_Wav_Read1024);
BENCHMARK(BM_Wav_Read10ms);
BENCHMARK(BM_AudioInput_Read10ms);
BENCHMARK(BM_AudioBuffer_Append10ms);
BENCHMARK(BM_AudioOutput_NullService10ms);
BENCHMARK(BM_AudioBuffer_PeekCommit10ms);
BENCHMARK(BM_AudioFrameBuilder_Build1024);
BENCHMARK(BM_AudioProcessor_None);
BENCHMARK(BM_AudioProcessor_Filter1);
BENCHMARK(BM_AudioProcessor_Filter2);
BENCHMARK(BM_AudioProcessor_FFT);
BENCHMARK(BM_AudioAnalyzer_Analyze1024);
BENCHMARK(BM_AcousticContext_Update);
BENCHMARK(BM_AudioVisualBridge_RunFrameNone);
BENCHMARK(BM_EndToEnd_Process10msWavToNullOutputToBridgeNone);
