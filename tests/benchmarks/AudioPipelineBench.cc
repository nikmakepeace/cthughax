#include "Audio.h"
#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"
#include "AudioTypes.h"
#include "AudioVisualBridge.h"

#include <benchmark/benchmark.h>

#include <limits.h>
#include <stddef.h>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static AudioDeviceConfig audioDeviceConfigValue;

AudioDeviceConfig::AudioDeviceConfig()
    : inputMode(AIM_File)
    , inputLoopEnabled(0)
    , pcmFormat()
    , dspMethod(0)
    , dspFragments(16)
    , dspFragmentSize(0)
    , dspSyncEnabled(0)
    , silentEnabled(0) {
    pcmFormat.sampleRate = 44100;
    pcmFormat.channels = 2;
    pcmFormat.sampleFormat = SF_s16_le;
    dspDevicePath[0] = '\0';
}

const AudioDeviceConfig& audioDeviceConfig() { return audioDeviceConfigValue; }
const PcmFormat& audioPcmFormat() { return audioDeviceConfigValue.pcmFormat; }
AudioInputMode audioInputModeValue() { return audioDeviceConfigValue.inputMode; }
int audioInputLoopEnabled() { return audioDeviceConfigValue.inputLoopEnabled; }
void audioSetInputLoopEnabled(int enabled) {
    audioDeviceConfigValue.inputLoopEnabled = enabled ? 1 : 0;
}
int audioSampleRateHz() { return audioDeviceConfigValue.pcmFormat.sampleRate; }
int audioChannels() { return audioDeviceConfigValue.pcmFormat.channels; }
int audioSampleFormat() { return audioDeviceConfigValue.pcmFormat.sampleFormat; }
void audioSetPcmFormat(const PcmFormat& format) {
    audioDeviceConfigValue.pcmFormat = format;
}
void audioSetSampleRateHz(int sampleRateHz) {
    audioDeviceConfigValue.pcmFormat.sampleRate = sampleRateHz;
}
void audioSetChannels(int channels) {
    audioDeviceConfigValue.pcmFormat.channels = channels;
}
void audioSetSampleFormat(int sampleFormat) {
    audioDeviceConfigValue.pcmFormat.sampleFormat = sampleFormat;
}
int audioBytesPerSample() {
    return audioDeviceConfigValue.pcmFormat.bytesPerSample();
}
int audioDspMethod() { return audioDeviceConfigValue.dspMethod; }
int audioDspFragments() { return audioDeviceConfigValue.dspFragments; }
int audioDspFragmentSize() { return audioDeviceConfigValue.dspFragmentSize; }
void audioSetDspFragment(int fragments, int fragmentSize) {
    audioDeviceConfigValue.dspFragments = fragments;
    audioDeviceConfigValue.dspFragmentSize = fragmentSize;
}
void audioSetDspFragments(int fragments) {
    audioDeviceConfigValue.dspFragments = fragments;
}
void audioSetDspFragmentSize(int fragmentSize) {
    audioDeviceConfigValue.dspFragmentSize = fragmentSize;
}
int audioDspSyncEnabled() { return audioDeviceConfigValue.dspSyncEnabled; }
int audioSilentEnabled() { return audioDeviceConfigValue.silentEnabled; }
const char* audioDspDevicePath() { return audioDeviceConfigValue.dspDevicePath; }
const char* audioChannelsText() { return audioChannels() == 1 ? "mono" : "stereo"; }
const char* audioSampleFormatText(int) { return "benchmark-format"; }
const char* audioSampleFormatText() { return audioSampleFormatText(audioSampleFormat()); }
const char* audioOnOffText(int enabled) { return enabled ? (char*)" on" : (char*)"off"; }

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
    audioSetPcmFormat(format);
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
    DecodedAudioHistory history(kVideoFrameSamples * 2, fixture.bytesPerSample(), kVideoFrameSamples);
    AudioFrameBuilder builder;

    applyFormat(fixture.format);
    frame.clear();
    history.appendDecodedPcm(fixture.frame1024.data(), kVideoFrameSamples);
    builder.build(frame, history, kVideoFrameSamples / 2);
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
    int previousLoop = audioInputLoopEnabled();
    audioSetInputLoopEnabled(1);
    AudioInput input(new WavPcmSource(primaryFixturePath()));
    int bytesPerSample = audioBytesPerSample();
    int sliceSamples = (audioSampleRateHz() * kAudioSliceMs) / 1000;
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
    audioSetInputLoopEnabled(previousLoop);
}

static void BM_DecodedAudioHistory_Append10ms(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
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
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioOutputStream stream(history);
    AudioNullOutput output;
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
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
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
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioFrameBuilder builder;
    AudioFrame frame;
    long long centerSample = kVideoFrameSamples;
    long long totalSamples = 0;

    applyFormat(fixture.format);
    primeHistory(history);

    for (auto _ : state) {
        builder.build(frame, history, centerSample);
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

static void BM_AudioProcessor_Analyze1024(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);

    for (auto _ : state) {
        AudioMetrics metrics = processor.analyze(frame.raw, int(sound_minnoise));
        benchmark::DoNotOptimize(metrics.amplitude);
    }
}

static void BM_AcousticContext_Update(benchmark::State& state) {
    AudioProcessor processor;
    AudioFrame frame;
    fillFrameFromFixture(frame);
    AudioMetrics metrics = processor.analyze(frame.raw, int(sound_minnoise));

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
        bridge.runFrame(frame);
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(frame.metrics.amplitude);
        benchmark::ClobberMemory();
    }

    audioFrameSetTestOverride(NULL);
}

static void BM_EndToEnd_Process10msWavToNullOutputToBridgeNone(benchmark::State& state) {
    const PcmFixture& fixture = pcmFixture();
    int previousLoop = audioInputLoopEnabled();
    audioSetInputLoopEnabled(1);
    AudioInput input(new WavPcmSource(primaryFixturePath()));
    DecodedAudioHistory history(kBufferCapacitySamples, fixture.bytesPerSample(),
        kProtectedHistorySamples);
    AudioOutputStream stream(history);
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
            history.clear();
            stream.reset(0);
            state.ResumeTiming();
            samples = input.read(inputChunk.data(), (int)inputChunk.size(),
                fixture.sliceSamples);
        }

        history.appendDecodedPcm(inputChunk.data(), samples);
        output.service(stream, outputChunk.data(), fixture.sliceSamples, 0);
        builder.build(frame, history, stream.submittedEndPosition());
        bridge.runFrame(frame);

        totalSamples += samples;
        benchmark::DoNotOptimize(frame.processedWaveData);
        benchmark::DoNotOptimize(frame.metrics.amplitude);
        benchmark::ClobberMemory();
    }

    audioFrameSetTestOverride(NULL);
    state.SetItemsProcessed(totalSamples);
    audioSetInputLoopEnabled(previousLoop);
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
BENCHMARK(BM_AudioProcessor_Analyze1024);
BENCHMARK(BM_AcousticContext_Update);
BENCHMARK(BM_AudioVisualBridge_RunFrameNone);
BENCHMARK(BM_EndToEnd_Process10msWavToNullOutputToBridgeNone);
