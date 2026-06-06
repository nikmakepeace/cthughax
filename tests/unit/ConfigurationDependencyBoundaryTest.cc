/** @file
 * Source-level dependency boundary checks for deglobalisation work.
 */

#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>

#ifndef CTHUGHA_SOURCE_DIR
#error CTHUGHA_SOURCE_DIR must point at the repository root
#endif

static std::string readSourceFile(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    assert(file.good());

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

static void assertSourceDoesNotContain(const char* relativePath,
    const char* token) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    if (!file.good())
        return;

    std::ostringstream contentsStream;
    contentsStream << file.rdbuf();
    std::string contents = contentsStream.str();
    if (contents.find(token) != std::string::npos)
        fprintf(stderr, "%s still contains `%s`\n", relativePath, token);
    assert(contents.find(token) == std::string::npos);
}

static void assertSourceDoesNotExist(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    if (file.good())
        fprintf(stderr, "%s still exists\n", relativePath);
    assert(!file.good());
}

static void assertSourceContains(const char* relativePath, const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) == std::string::npos)
        fprintf(stderr, "%s does not contain `%s`\n", relativePath, token);
    assert(contents.find(token) != std::string::npos);
}

static void testAudioIngestUsesAudioConfig() {
    assertSourceDoesNotExist("src/AudioRuntime.cc");
    assertSourceDoesNotExist("src/AudioRuntime.h");
    assertSourceContains("src/AudioIngest.cc",
        "AudioSettings::fromConfig(configValue, *log)");
    assertSourceContains("src/AudioIngest.cc", "RuntimeFactory runtimeFactory");
    assertSourceDoesNotContain("src/AudioIngest.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioIngest.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioIngest.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundSampleRate");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundChannels");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundFormat");
    assertSourceContains("src/ProcessServices.h", "class SecondsClock");
    assertSourceContains("src/Application.h", "SystemSecondsClock secondsClockValue");
    assertSourceContains("src/Application.cc",
        "randomSourceValue,\n        secondsClockValue");
    assertSourceContains("src/AudioIngest.h", "SecondsClock& clock_");
    assertSourceDoesNotContain("src/AudioIngest.h", "AudioIngestClock");
    assertSourceDoesNotContain("src/AudioIngest.h", "SystemAudioIngestClock");
    assertSourceDoesNotContain("src/AudioIngest.h", "ownedClock");
    assertSourceDoesNotContain("src/AudioIngest.cc", "new SystemAudioIngestClock");
    assertSourceDoesNotContain("src/AudioIngest.cc", "getTime()");
    assertSourceDoesNotContain("src/AudioSettings.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioSettings.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::soundDSPMethod");
    assertSourceDoesNotContain("src/AudioSettings.cc", "soundSilent");
    assertSourceContains("src/AudioSettings.h",
        "static AudioSettings fromConfig(const AudioConfig& config, LogSink& log)");
    assertSourceContains("src/AudioSettings.cc", "log.debug");
    assertSourceDoesNotContain("src/AudioSettings.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/AudioSettings.cc", "#include \"cthugha.h\"");
}

static void testAudioDeviceSettingsAreStartupOnly() {
    assertSourceDoesNotContain("src/AudioOptions.h", "struct AudioDeviceConfig");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioDeviceConfig");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioPcmFormat");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioInputModeValue");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioInputLoopEnabled");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioSet");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioSampleRateHz");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioChannels()");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioSampleFormat()");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioBytesPerSample");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioDsp");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioSilentEnabled");
    assertSourceDoesNotContain("src/AudioOptions.h", "audioDspDevicePath");
    assertSourceContains("src/AudioOptions.h", "audioChannelsText(int channels)");
    assertSourceContains("src/AudioOptions.h",
        "audioSampleFormatText(int sampleFormat)");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& audioInputMode");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundFormat");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundChannels");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundSampleRate");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPMethod");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPFragments");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPFragmentSize");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPSync");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundSilent");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern int audioInputLoop");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern char dev_dsp");
    assertSourceDoesNotContain("src/AudioSystem.cc", "InterfaceElementOption");
    assertSourceDoesNotContain("src/AudioSystem.cc", "interfaceAudio");
    assertSourceDoesNotContain("src/AudioSystem.cc", "Audio Interface");
    assertSourceDoesNotContain("src/AudioSystem.cc", "audioFrameChange");
    assertSourceDoesNotContain("src/AudioSystem.cc", "AudioDeviceConfig");
    assertSourceDoesNotContain("src/AudioSystem.cc", "audioDeviceConfigValue");
    assertSourceDoesNotContain("src/AudioSystem.cc", "configureAudioOptions");
    assertSourceContains("src/AudioSystem.cc", "audioChannelsText(int channels)");
    assertSourceContains("src/Mixer.h", "class MixerDevice");
    assertSourceContains("src/Mixer.h", "class MixerSession");
    assertSourceContains("src/Mixer.h", "class MixerControls");
    assertSourceDoesNotContain("src/Mixer.h", "extern char dev_mixer");
    assertSourceDoesNotContain("src/Mixer.h", "configureMixer");
    assertSourceDoesNotContain("src/Mixer.h", "init_mixer");
    assertSourceDoesNotContain("src/Mixer.h", "mixer_initial_volume");
    assertSourceContains("src/Mixer.cc",
        "MixerInitialVolume(it->name, it->volume)");
    assertSourceContains("src/Mixer.h", "class LogSink");
    assertSourceContains("src/Mixer.h",
        "MixerSession(MixerDevice& device, LogSink& log");
    assertSourceContains("src/Mixer.h",
        "MixerControls(MixerSession& session, LogSink& log)");
    assertSourceContains("src/Mixer.cc", "log.errorErrno");
    assertSourceContains("src/Mixer.cc", "log.warn");
    assertSourceContains("src/MixerControls.cc", "logValue.error");
    assertSourceDoesNotContain("src/Mixer.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/MixerControls.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/Mixer.cc", "CTH_");
    assertSourceDoesNotContain("src/MixerControls.cc", "CTH_");
    assertSourceDoesNotContain("src/MixerControls.cc", "static char str");
    assertSourceContains("src/MixerControls.cc", "mutable char textValue[128]");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<MixerSession> mixerSessionValue");
    assertSourceContains("src/Application.cc", "newMixerDevice()");
    assertSourceContains("src/Application.cc",
        "new MixerSession(*mixerDeviceValue, logSinkValue");
    assertSourceContains("src/Application.cc",
        "new MixerControls(*mixerSessionValue, logSinkValue)");
    assertSourceContains("src/Application.cc",
        "mixerControlsValue->installInto(interfaceMixer)");
    assertSourceContains("src/AudioSettings.h", "PcmFormat pcmFormat");
    assertSourceContains("src/AudioIngest.h", "PcmFormat pcmFormatValue");
    assertSourceContains("src/AudioIngest.cc",
        "pcmFormatValue = (inputValue.get() != NULL)");
    assertSourceContains("src/AudioIngest.cc",
        "new DecodedAudioHistory(capacitySamples, pcmFormatValue");
    assertSourceContains("src/RuntimeFactory.h",
        "AudioOutput* createAudioOutput(const PcmFormat& format) const");
    assertSourceContains("src/RuntimeFactory.h",
        "static Environment detect(const AudioSettings& settings, LogSink& log)");
    assertSourceContains("src/RuntimeFactory.h", "RandomSource& randomSource");
    assertSourceContains("src/RuntimeFactory.h", "LogSink& log");
    assertSourceContains("src/AudioIngest.h", "RandomSource& randomSource_");
    assertSourceContains("src/AudioIngest.h", "LogSink& log_");
    assertSourceContains("src/AudioIngest.cc", "log->debug");
    assertSourceContains("src/Application.h", "ConsoleLogSink logSinkValue");
    assertSourceContains("src/Application.cc",
        "CthughaBuffer::buffer.maxDimension(), randomSourceValue");
    assertSourceContains("src/Application.cc", "secondsClockValue, logSinkValue");
    assertSourceContains("src/PcmSource.cc", "AudioInput::format() const");
    assertSourceContains("src/Audio.h", "PcmSource(LogSink& log_)");
    assertSourceContains("src/Audio.h",
        "AudioInput(PcmSource* source, LogSink& log");
    assertSourceContains("src/PcmSource.cc", "AudioInput::AudioInput");
    assertSourceContains("src/PcmSource.cc", "log.debug");
    assertSourceContains("src/PcmSource.cc", "log.warn");
    assertSourceContains("src/PcmSource.cc", "log.info");
    assertSourceContains("src/PcmSource.cc", "log.error");
    assertSourceContains("src/PcmSource.cc", "log.errorErrno");
    assertSourceContains("src/PcmSource.cc",
        "RandomNoisePcmSource::RandomNoisePcmSource(const PcmFormat& requestedFormat,");
    assertSourceContains("src/PcmSourceFactory.cc",
        "new RandomNoisePcmSource(settings.pcmFormat, randomSource, log)");
    assertSourceContains("src/PcmSourceFactory.cc",
        "new WavPcmSource(settings.fileName, log)");
    assertSourceContains("src/PcmSourceFactory.cc", "#include \"config.h\"");
    assertSourceContains("src/PcmSourceFactory.cc",
        "new Minimp3PcmSource(settings.fileName, log)");
    assertSourceContains("src/PcmSourceFactory.cc",
        "new RawPcmSource(settings.fileName, settings.pcmFormat, log)");
    assertSourceContains("src/PcmSourceFactory.cc",
        "new DspPcmSource(settings, visualMaxDimension, log)");
    assertSourceContains("src/RuntimeFactory.cc",
        "new AudioInput(source, log, 1, settings.inputLoopEnabled)");
    assertSourceDoesNotContain("src/PcmSource.cc", "rand()");
    assertSourceContains("tests/unit/AudioDeviceNegotiationTest.cc",
        "testRandomNoiseUsesInjectedRandomSource");
    assertSourceContains("tests/CMakeLists.txt",
        "audio_device_negotiation_test");
    assertSourceDoesNotContain("src/PcmSource.cc", "audioSetPcmFormat");
    assertSourceDoesNotContain("src/PcmSource.cc", "audioSampleRateHz");
    assertSourceDoesNotContain("src/PcmSource.cc", "audioChannels");
    assertSourceDoesNotContain("src/PcmSource.cc", "audioSampleFormat");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "audioSetSampleRateHz");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "audioSetChannels");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "audioSetSampleFormat");
    assertSourceContains("src/DspPcmSource.cc", "DspPcmSource::DspPcmSource");
    assertSourceContains("src/DspPcmSource.cc", "LogSink& log_");
    assertSourceContains("src/DspPcmSource.cc", "log.debug");
    assertSourceContains("src/DspPcmSource.cc", "log.warn");
    assertSourceContains("src/DspPcmSource.cc", "log.info");
    assertSourceContains("src/DspPcmSource.cc", "log.error");
    assertSourceContains("src/DspPcmSource.cc", "log.errorErrno");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "audioSetSampleRateHz");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "audioSetChannels");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "audioSetSampleFormat");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "audioSampleRateHz");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "audioChannels");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "audioSampleFormat");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "audioBytesPerSample");
    assertSourceContains("src/Audio.h",
        "DecodedAudioHistory(int capacitySamples, const PcmFormat& format,\n"
        "        int retainedHistorySamples, LogSink& log_)");
    assertSourceContains("src/Audio.h", "explicit AudioFrameBuilder(LogSink& log_)");
    assertSourceContains("src/DecodedAudioHistory.cc", "log.debug");
    assertSourceContains("src/DecodedAudioHistory.cc", "log.trace");
    assertSourceContains("src/DecodedAudioHistory.cc", "log.error");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "CTH_");
    assertSourceDoesNotContain("src/DecodedAudioHistory.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/RuntimeFactory.cc", "audioDspDevicePath");
    assertSourceDoesNotContain("src/AudioIngest.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/AudioIngest.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/AudioIngest.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/AudioIngest.cc", "CTH_LOG_ENABLED");
    assertSourceDoesNotContain("src/AudioIngest.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/RuntimeFactory.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/PcmSourceFactory.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/PcmSourceFactory.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/PcmSource.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/PcmSource.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/PcmSource.cc", "CTH_INFO");
    assertSourceDoesNotContain("src/PcmSource.cc", "CTH_ERROR");
    assertSourceDoesNotContain("src/PcmSource.cc", "CTH_ERRNO");
    assertSourceDoesNotContain("src/PcmSource.cc", "cth_log");
    assertSourceDoesNotContain("src/PcmSource.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "CTH_INFO");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "CTH_ERROR");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "CTH_ERRNO");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "cth_log");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/Audio.h", "initInputControls");
    assertSourceDoesNotContain("src/AudioIngest.cc", "initInputControls");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "initInputControls");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "init_mixer");
    assertSourceDoesNotContain("src/DspPcmSource.cc", "#include \"Mixer.h\"");
    assertSourceContains("tests/CMakeLists.txt", "mixer_session_test");
    assertSourceContains("tests/CMakeLists.txt", "mixer_controls_test");
}

static void testAudioOutputSettingsAreSessionOwned() {
    assertSourceContains("src/AudioOutputConfig.h", "struct AudioOutputConfig");
    assertSourceContains("src/AudioOutputDump.h", "class AudioOutputDump");
    assertSourceContains("src/AudioIngest.h",
        "std::unique_ptr<AudioOutputDump> outputDumpValue");
    assertSourceContains("src/AudioIngest.cc",
        "AudioOutputConfig::fromConfig(configValue)");
    assertSourceContains("src/RuntimeFactory.h",
        "AudioOutputConfig outputConfig");
    assertSourceContains("src/RuntimeFactory.h", "SecondsClock& clock");
    assertSourceContains("src/RuntimeFactory.h", "LogSink& log");
    assertSourceContains("src/RuntimeFactory.cc",
        "new AudioNullOutput(clock, log, outputConfig, outputDump)");
    assertSourceContains("src/RuntimeFactory.cc",
        "new AudioPulseOutput(format, clock, log,");
    assertSourceContains("src/RuntimeFactory.cc",
        "new AudioDSPOutput(settings, outputConfig,\n"
        "            visualMaxDimension, clock, log, outputDump)");
    assertSourceContains("src/AudioOutput.cc",
        "AudioOutput::AudioOutput(int targetLatencyMs, AudioOutputDump* outputDump,\n"
        "    SecondsClock& clock_, LogSink& log_)");
    assertSourceContains("src/AudioOutput.cc", "clock.nowSeconds()");
    assertSourceContains("src/AudioOutput.cc", "log.debug");
    assertSourceContains("src/AudioOutput.cc", "log.warn");
    assertSourceContains("src/AudioOutput.cc", "log.trace");
    assertSourceContains("src/AudioInternal.cc",
        "AudioSubmittedPcmDebugReporter::submittedPcm");
    assertSourceContains("src/AudioInternal.cc", "LogSink& log");
    assertSourceContains("src/AudioInternal.cc", "log.debug");
    assertSourceContains("src/AudioPulseOutput.cc", "logSink().debug");
    assertSourceContains("src/AudioPulseOutput.cc", "logSink().warn");
    assertSourceContains("src/AudioPulseOutput.cc", "logSink().error");
    assertSourceContains("src/AudioPulseOutput.cc", "logSink().trace");
    assertSourceContains("src/AudioDSPOutput.cc", "logSink().debug");
    assertSourceContains("src/AudioDSPOutput.cc", "logSink().info");
    assertSourceContains("src/AudioDSPOutput.cc", "logSink().error");
    assertSourceContains("src/AudioDSPOutput.cc", "logSink().errorErrno");
    assertSourceContains("src/AudioPassthrough.h", "LogSink& log");
    assertSourceContains("src/AudioPassthrough.cc", "log.debug");
    assertSourceContains("src/AudioOutputDump.h", "LogSink& log");
    assertSourceContains("src/AudioOutputDump.cc", "log.warn");
    assertSourceContains("src/AudioOutputDump.cc", "log.errorErrno");
    assertSourceContains("src/AudioOutputDump.cc", "log.debug");
    assertSourceDoesNotContain("src/AudioOutput.cc", "getTime()");
    assertSourceDoesNotContain("src/AudioOutput.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/AudioOutput.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/AudioOutput.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/AudioOutput.cc", "CTH_LOG_ENABLED");
    assertSourceDoesNotContain("src/AudioOutput.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioInternal.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/AudioInternal.cc", "CTH_LOG_ENABLED");
    assertSourceDoesNotContain("src/AudioInternal.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioPulseOutput.cc", "CTH_");
    assertSourceDoesNotContain("src/AudioPulseOutput.cc", "cth_log");
    assertSourceDoesNotContain("src/AudioPulseOutput.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "CTH_");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "cth_log");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioPassthrough.cc", "CTH_");
    assertSourceDoesNotContain("src/AudioPassthrough.cc", "cth_log");
    assertSourceDoesNotContain("src/AudioPassthrough.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioOutputDump.cc", "CTH_");
    assertSourceDoesNotContain("src/AudioOutputDump.cc", "cth_log");
    assertSourceDoesNotContain("src/AudioOutputDump.cc", "#include \"cthugha.h\"");
    assertSourceContains("src/AudioOutput.cc",
        "outputDumpValue->append(format, data, bytes)");
    assertSourceContains("tests/CMakeLists.txt", "audio_output_config_test");
    assertSourceContains("tests/CMakeLists.txt", "audio_output_dump_test");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern char pulse_server");
    assertSourceDoesNotContain("src/AudioOptions.h", "pulse_latency_msec");
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_output_dump");
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_null_target_latency_msec");
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_pulse_target_latency_msec");
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_dsp_target_latency_msec");
    assertSourceDoesNotContain("src/AudioOutput.cc", "char pulse_server");
    assertSourceDoesNotContain("src/AudioOutput.cc", "pulse_latency_msec");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_output_dump");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_null_target_latency_msec");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_pulse_target_latency_msec");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_dsp_target_latency_msec");
    assertSourceDoesNotContain("src/AudioInternal.cc", "audio_output_dump");
    assertSourceDoesNotContain("src/RuntimeFactory.cc", "pulse_server");
    assertSourceDoesNotContain("src/AudioPulseOutput.cc", "pulse_server");
    assertSourceDoesNotContain("src/AudioPulseOutput.cc", "pulse_latency_msec");
    assertSourceDoesNotContain("src/AudioDSPOutput.cc", "audio_dsp_target_latency_msec");
    assertSourceDoesNotContain("src/AudioSystem.cc", "configureAudioOutputOptions");
}

static void testAudioFrameOwnsPerFrameMetrics() {
    assertSourceContains("src/AudioFrame.h", "struct AudioMetrics");
    assertSourceContains("src/AudioFrame.h", "AudioMetrics metrics");
    assertSourceContains("src/AudioProcessor.cc",
        "AudioProcessor::analyze(const char2* frame, int minNoise)");
    assertSourceContains("src/AudioProcessing.h", "class AudioProcessingState");
    assertSourceContains("src/AudioProcessing.h", "class AudioProcessingSelector");
    assertSourceContains("src/AudioProcessing.h", "RandomSource& randomSource");
    assertSourceContains("src/AudioProcessing.h", "LogSink& log");
    assertSourceContains("src/Application.cc",
        "new AudioProcessingState(randomSourceValue)");
    assertSourceContains("src/Application.cc",
        "*audioProcessorValue, logSinkValue)");
    assertSourceDoesNotContain("src/AudioProcessingState.cc", "rand()");
    assertSourceContains("tests/unit/RuntimeAudioControlsTest.cc",
        "testUnknownAudioProcessingSelectionUsesInjectedRandomSource");
    assertSourceContains("src/AudioFftProcessor.h", "class AudioFftProcessor");
    assertSourceContains("src/AudioFftProcessor.h",
        "class FixedPointAudioFftProcessor");
    assertSourceDoesNotContain("src/AudioFftProcessor.cc",
        "static FixedPointAudioFftProcessor processor");
    assertSourceDoesNotContain("src/AudioFftProcessor.h",
        "defaultAudioFftProcessor");
    assertSourceContains("src/Audio.h",
        "FixedPointAudioFftProcessor defaultFftProcessorValue");
    assertSourceContains("src/Audio.h",
        "explicit AudioProcessor(AudioFftProcessor& fftProcessor)");
    assertSourceContains("src/AudioProcessor.cc",
        "fftProcessorValue(&defaultFftProcessorValue)");
    assertSourceContains("src/AudioProcessor.cc",
        "fftProcessorValue->transform(raw, processedWaveData)");
    assertSourceContains("tests/unit/AudioFrameProcessorTest.cc",
        "testAudioProcessorDelegatesFftToInjectedProcessor");
    assertSourceContains("tests/unit/AudioFrameProcessorTest.cc",
        "testFixedPointFftProducesCharacterizedOutput");
    assertSourceContains("tests/benchmarks/AudioPipelineBench.cc",
        "prism-2s-48000-stereo-s16le.raw");
    assertSourceContains("tests/benchmarks/AudioPipelineBench.cc",
        "BM_AudioFft_FixedPoint_PrismPcm");
    assertSourceContains("tests/benchmarks/CMakeLists.txt", "CTH_DISABLE_MINIMP3");
    assertSourceDoesNotContain("src/AudioFftProcessor.h",
        "LookupTableAudioFftProcessor");
    assertSourceDoesNotContain("tests/benchmarks/AudioPipelineBench.cc",
        "LookupTableAudioFftProcessor");
    assertSourceContains("tests/CMakeLists.txt", "AudioFftProcessor.cc");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorWp");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorR");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorFftInit");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "initAudioProcessorFft");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioFrame.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioSystem.cc", "#include \"cthugha.h\"");
    assertSourceContains("src/AudioFramePipeline.cc",
        "audioProcessingSelectorValue.process(frame)");
    assertSourceContains("src/AudioFramePipeline.cc",
        "audioProcessorValue.analyze(frame, minNoiseValue)");
    assertSourceContains("src/AudioFramePipeline.h", "class AudioFramePipeline");
    assertSourceContains("src/AudioFramePipeline.h",
        "class DefaultAudioFramePipeline");
    assertSourceDoesNotExist("src/AudioVisualBridge.h");
    assertSourceDoesNotExist("src/AudioVisualBridge.cc");
    assertSourceDoesNotContain("src/Application.h", "AudioVisualBridge");
    assertSourceDoesNotContain("src/Application.cc", "AudioVisualBridge");
    assertSourceDoesNotContain("src/Application.cc",
        "filterchainRefreshRequested");
    assertSourceContains("src/AudioFramePipeline.cc", "clock.nowSeconds()");
    assertSourceContains("src/AudioFramePipeline.cc", "log.debug");
    assertSourceContains("src/AudioFramePipeline.cc", "log.trace");
    assertSourceContains("src/AudioProcessingSelector.cc",
        "log.trace(\"audio processing");
    assertSourceDoesNotContain("src/AudioProcessingSelector.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/AudioProcessingSelector.cc",
        "#include \"cthugha.h\"");
    assertSourceContains("src/AudioAnalyzer.h", "explicit AcousticContext(LogSink*");
    assertSourceContains("src/AudioAnalyzer.cc", "log->trace(\"sound fire\"");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc", "getTime()");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc", "CTH_LOG_ENABLED");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc",
        "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AudioFramePipeline.cc",
        "static AudioProcessor");
    assertSourceDoesNotContain("src/AudioProcessor.cc",
        "static AudioProcessor");
    assertSourceContains("src/AudioFramePipeline.cc",
        "acousticContextValue.update(frame.metrics)");
    assertSourceContains("src/Application.h",
        "AcousticContext acousticContextValue");
    assertSourceContains("src/Application.cc",
        "new DefaultAudioFramePipeline(");
    assertSourceContains("src/Application.cc",
        "audioFramePipelineValue->processFrame(frame)");
    assertSourceDoesNotContain("tests/benchmarks/AudioPipelineBench.cc",
        "AudioVisualBridge");
    assertSourceDoesNotContain("tests/CMakeLists.txt",
        "audio_visual_bridge_frame_test");
    assertSourceDoesNotContain("tests/benchmarks/CMakeLists.txt",
        "AudioVisualBridge.cc");
    assertSourceContains("src/Application.cc",
        "initAudioFramePipeline()");
    assertSourceContains("src/Application.cc",
        "runAudioFramePipeline(audioFrame)");
    assertSourceContains("src/Application.cc",
        "*audioProcessorValue, secondsClockValue, logSinkValue");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.audioAnalysis.minNoise));");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.audioAnalysis.minNoise");
    assertSourceContains("src/Application.cc",
        "context.audioMetrics = &frame.metrics");
    assertSourceContains("src/Application.cc",
        "context.acousticContext = &acousticContext");
    assertSourceContains("src/Application.cc",
        "displayValue->present(*indexedFrame, presentationContext)");
    assertSourceContains("src/ScreenRenderContext.h",
        "const AudioMetrics* audioMetrics() const");
    assertSourceContains("src/ScreenRenderContext.h",
        "const AcousticContext* acousticContext() const");
    assertSourceContains("src/PresentationComposer.cc",
        "ScreenRenderContext(source, destination, frameTimeSeconds,");
    assertSourceContains("src/AutoChanger.cc",
        "void AutoChanger::operator()(const AudioMetrics& metrics)");
    assertSourceContains("src/AutoChanger.h",
        "AcousticContext& acousticContext_");
    assertSourceContains("src/AutoChanger.h", "class AutoChangeQuietObserver");
    assertSourceContains("src/AutoChanger.h",
        "AutoChangeQuietObserver& quietObserver_");
    assertSourceContains("src/AutoChanger.h", "LogSink& log_");
    assertSourceContains("src/AutoChanger.cc", "log.debug");
    assertSourceContains("src/AutoChanger.cc",
        "quietObserver.observeQuiet(quiet_length)");
    assertSourceDoesNotContain("src/AutoChanger.cc", "videoDirector()");
    assertSourceDoesNotContain("src/AutoChanger.cc", "#include \"VideoDirector.h\"");
    assertSourceContains("src/AutoChanger.h", "mutable char statusTextValue[512]");
    assertSourceDoesNotContain("src/AutoChanger.cc", "static char txt");
    assertSourceDoesNotContain("src/AutoChanger.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AutoChanger.cc", "CTH_DEBUG");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<AutoChanger> autoChangerValue");
    assertSourceContains("src/Application.cc",
        "autoChangerValue.reset(new AutoChanger");
    assertSourceContains("src/Application.cc",
        "(*autoChangerValue)(frame.metrics)");
    assertSourceContains("src/AutoChanger.h",
        "class AutoChanger : public AutoChangerStatusProvider");
    assertSourceContains("src/InterfaceRuntime.h",
        "const AutoChangerStatusProvider* autoChangerStatusProviderValue");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setAutoChangerStatusProvider(autoChangerValue.get())");
    assertSourceContains("src/Application.cc", "class VideoDirectorQuietObserver");
    assertSourceContains("src/Application.cc",
        "autoChangeQuietObserverValue.reset(");
    assertSourceContains("src/Application.cc",
        "*autoChangeQuietObserverValue, logSinkValue));");
    assertSourceContains("tests/CMakeLists.txt",
        "audio_frame_processor_test");
    assertSourceContains("tests/CMakeLists.txt",
        "audio_frame_pipeline_test");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameSetCurrent");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameCurrent");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameRawData");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFramePublishMetrics");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameMetrics");
    assertSourceDoesNotContain("src/AudioFrame.cc", "currentAudioFrame");
    assertSourceDoesNotContain("src/Application.cc", "audioFrameSetCurrent");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameCurrent");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameRawData");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AutoChanger.cc", "audioFrameMetrics");
    assertSourceDoesNotContain("src/Border.cc", "audioFrameRawData");
    assertSourceDoesNotContain("src/display.cc", "audioFrameMetrics");
    assertSourceDoesNotContain("src/display.cc", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "class AudioAnalyzer");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "extern AudioMetrics audioMetrics");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "extern AcousticContext acousticContext");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AudioAnalyzer::");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AudioMetrics audioMetrics");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AcousticContext acousticContext");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "sound_minnoise");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/Interface.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/AutoChanger.cc", "audioMetrics.");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern AutoChanger* autoChanger");
    assertSourceDoesNotContain("src/AutoChanger.cc", "AutoChanger* autoChanger");
    assertSourceDoesNotContain("src/Interface.cc", "autoChanger->");
    assertSourceDoesNotContain("src/display.cc", "audioMetrics.");
}

static void testDisplayStartupUsesDisplayConfig() {
    assertSourceContains("src/Application.cc", "startupConfigValue.display");
    assertSourceContains("src/DisplayDeviceX11.cc", "checkDisplaySize(config)");
    assertSourceDoesNotContain("src/DisplayDevice.h", "display_mode");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "display_mode");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "display_mode");
}

static void testAutoChangeSettingsAreApplicationOwned() {
    assertSourceContains("src/Application.h",
        "std::unique_ptr<AutoChangeSettings> autoChangeSettingsValue");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<AutoChangeControls> autoChangeControlsValue");
    assertSourceContains("src/Application.cc",
        "new OwnedAutoChangeSettings(startupConfigValue.autoChange)");
    assertSourceContains("src/Application.cc",
        "new AutoChangeControls(*autoChangeSettingsValue, logSinkValue)");
    assertSourceContains("src/AutoChangeControls.h", "class LogSink");
    assertSourceContains("src/AutoChangeControls.h",
        "AutoChangeControls(AutoChangeSettings& settings, LogSink& log)");
    assertSourceContains("src/AutoChangeControls.cc", "log.error");
    assertSourceDoesNotContain("src/AutoChangeControls.cc",
        "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/AutoChangeControls.cc", "CTH_");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue)");
    assertSourceContains("src/Application.cc",
        "videoDirector().setRandomSource(randomSourceValue)");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setAutoChangeControls(autoChangeControlsValue.get())");
    assertSourceContains("src/Application.cc",
        "autoChangerValue.reset(new AutoChanger(*runtimeChangeMediatorValue");
    assertSourceContains("src/Application.cc",
        "millisecondClockValue, randomSourceValue");
    assertSourceContains("src/Application.h",
        "SystemMillisecondClock millisecondClockValue");
    assertSourceContains("src/Application.h",
        "CStdRandomSource randomSourceValue");
    assertSourceContains("src/Application.cc",
        "millisecondClockValue, randomSourceValue");
    assertSourceContains("src/ProcessServices.h", "unsigned int stateValue");
    assertSourceContains("src/ProcessServices.cc",
        "CStdRandomSource::CStdRandomSource(unsigned int seed_)");
    assertSourceContains("src/ProcessServices.cc", "std::chrono::steady_clock");
    assertSourceDoesNotContain("src/ProcessServices.cc", "gettime()");
    assertSourceDoesNotContain("src/ProcessServices.cc", "getTime()");
    assertSourceDoesNotContain("src/ProcessServices.h", "seedLegacyProcessRandom");
    assertSourceDoesNotContain("src/ProcessServices.cc", "seedLegacyProcessRandom");
    assertSourceDoesNotContain("src/Application.cc", "seedLegacyProcessRandom");
    assertSourceDoesNotContain("src/imath.h", "Random(int range)");
    assertSourceDoesNotContain("src/imath.cc", "int Random(int range)");
    assertSourceDoesNotContain("src/ProcessServices.cc", "rand()");
    assertSourceDoesNotContain("src/Application.cc", "srand(");
    assertSourceDoesNotContain("src/Application.cc", "time(0)");
    assertSourceContains("tests/unit/ProcessServicesTest.cc",
        "testSeededRandomSourcesAreDeterministicAndIndependent");
    assertSourceContains("src/AutoChanger.h", "MillisecondClock& clock");
    assertSourceContains("src/AutoChanger.h", "RandomSource& randomSource");
    assertSourceDoesNotContain("src/AutoChanger.cc", "gettime()");
    assertSourceDoesNotContain("src/AutoChanger.cc", "rand()");
    assertSourceContains("src/VideoDirector.h", "RandomSource* randomSourceValue");
    assertSourceContains("src/VideoDirector.cc",
        "silenceMessage.setRandomSource(randomSource)");
    assertSourceDoesNotContain("src/VideoDirector.cc", "rand()");
    assertSourceContains("src/SilenceMessage.h", "RandomSource* randomSourceValue");
    assertSourceDoesNotContain("src/SilenceMessage.cc", "rand()");
    assertSourceDoesNotContain("src/DefaultMessagesProvider.cc", "rand()");
    assertSourceDoesNotContain("src/FileMessagesProvider.cc", "rand()");
    assertSourceContains("tests/unit/MessagesProviderTest.cc",
        "testDefaultMessagesUseInjectedRandomSource");
    assertSourceContains("src/Application.cc",
        "*autoChangeSettingsValue");
    assertSourceContains("src/AutoChanger.h",
        "const AutoChangeSettings& settings");
    assertSourceContains("src/AutoChanger.cc", "settings.quietMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.waitMinMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.waitRandomMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.cumulativeFireLevel()");
    assertSourceContains("src/AutoChanger.cc", "settings.locked()");
    assertSourceContains("src/AutoChanger.cc", "settings.changeLittle()");
    assertSourceContains("src/RuntimeAutoChangeControls.cc",
        "autoChangeControls.changeOptionBy(option, by)");
    assertSourceContains("src/Interface.cc",
        "InterfaceElementAutoChangeOption");
    assertSourceContains("src/Interface.cc",
        "controls->option(field)");
    assertSourceContains("src/LegacyRuntimeConfigContributor.cc",
        "config.autoChange = autoChangeSettings.config()");
    assertSourceContains("tests/CMakeLists.txt",
        "runtime_auto_change_controls_test");
    assertSourceContains("tests/unit/RuntimeAutoChangeControlsTest.cc",
        "class FakeAutoChangeSettings");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeQuiet");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionOnOff lock");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionOnOff change_little");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeQuiet");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionOnOff lock");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionOnOff change_little");
    assertSourceDoesNotContain("src/Application.cc",
        "configureAutoChanger(startupConfigValue.autoChange)");
    assertSourceDoesNotContain("src/Interface.cc", "&changeWaitMin");
    assertSourceDoesNotContain("src/Interface.cc", "&changeWaitRandom");
    assertSourceDoesNotContain("src/Interface.cc", "&changeQuiet");
    assertSourceDoesNotContain("src/Interface.cc", "&changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/Interface.cc", "&change_little");
    assertSourceDoesNotContain("src/Interface.cc", "&lock");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeWaitMin");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeWaitRandom");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeQuiet");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "change_little");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeWaitMin.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeWaitRandom.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeQuiet.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc",
        "changeCumulativeFireLevel.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "change_little.text()");
}

static void testX11StartupUsesX11ConfigOnlyForX11Builds() {
    assertSourceContains("src/Configuration.h", "#ifdef CTH_XWIN\nstruct X11Config");
    assertSourceContains("src/Configuration.h", "#ifdef CTH_XWIN\n    X11Config x11;");
    assertSourceContains("src/Application.cc",
        "configureDisplayDeviceX11(startupConfigValue.x11)");
    assertSourceDoesNotContain("src/Application.cc",
        "configureDisplayDevice(startupConfigValue.display)");
    assertSourceContains("src/xcthugha.h",
        "void configureDisplayDeviceX11(const X11Config& config)");
    assertSourceContains("src/DisplayDeviceX11.cc",
        "void configureDisplayDeviceX11(const X11Config& config)");
    assertSourceContains("src/DisplayDeviceX11.cc", "config.frameDumpDirectory");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "getenv(\"CTHUGHA_DUMP_X11");
    assertSourceDoesNotContain("src/Configuration.h", "textOnTerm");
    assertSourceDoesNotContain("src/Configuration.h", "ncursesEnabled");
    assertSourceDoesNotContain("src/Configuration.cc", "text-on-term");
    assertSourceDoesNotContain("src/Configuration.cc", "KEY_X11_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/configuration_defaults.h", "NCURSES");
    assertSourceDoesNotContain("src/configuration_defaults.h",
        "X11_CONFIG_DEFAULT_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/DisplayDevice.h", "text_on_term");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "text_on_term");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "text_on_term");
    assertSourceDoesNotContain("src/info_title_usage.cc", "text-on-term");
    assertSourceDoesNotContain("src/keys.cc", "getkey_ncurs");
    assertSourceDoesNotContain("src/keys.cc", "ncurses");
    assertSourceDoesNotContain("src/display.h", "init_ncurses");
    assertSourceDoesNotContain("src/display.h", "exit_ncurses");
    assertSourceDoesNotExist("src/disp-ncurses.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt", "disp-ncurses.cc");
    assertSourceDoesNotContain("CMakeLists.txt", "find_package(Curses");
    assertSourceDoesNotContain("CMakeLists.txt", "HAVE_NCURSES");
    assertSourceDoesNotContain("cmake/config.h.in", "HAVE_NCURSES");
    assertSourceDoesNotContain("src/Configuration.h", "x11FontName");
    assertSourceDoesNotContain("src/Configuration.h", "x11MitShm");
    assertSourceDoesNotContain("src/options.cc", "&display_mit_shm");
    assertSourceDoesNotContain("src/options.cc", "&display_on_root");
    assertSourceDoesNotContain("src/options.cc", "&private_cmap");
    assertSourceDoesNotContain("src/options.cc", "&display_override_redirect");
    assertSourceDoesNotContain("src/options.cc", "&xcth_panel");
    assertSourceDoesNotContain("src/options.cc", "&DisplayDevice::text_on_term");
    assertSourceDoesNotContain("src/options.cc", "window_pos.x");
    assertSourceDoesNotContain("src/options.cc", "strncpy(xcth_font");
}

static void testLoggingUsesLoggingConfig() {
    assertSourceContains("src/Application.h", "LoggingRuntime loggingRuntimeValue");
    assertSourceContains("src/Application.h", "ConsoleLogSink logSinkValue");
    assertSourceContains("src/Application.cc",
        "cthugha_install_logging_runtime(loggingRuntimeValue)");
    assertSourceContains("src/Application.cc",
        "loggingRuntimeValue.configure(startupConfigValue.logging)");
    assertSourceContains("src/Application.cc",
        "cthugha_clear_logging_runtime(loggingRuntimeValue)");
    assertSourceContains("src/Application.cc", "logSinkValue.info");
    assertSourceContains("src/Application.cc", "log.warn");
    assertSourceContains("src/Application.cc", "log.error");
    assertSourceContains("src/Application.cc", "logSinkValue.trace");
    assertSourceContains("src/Application.cc", "logSinkValue.traceEnabled()");
    assertSourceContains("src/Application.h",
        "DisplayFrontendInitializer* displayFrontendInitializer");
    assertSourceContains("src/Application.h",
        "LegacyDisplayFrontendInitializer displayFrontendInitializerValue");
    assertSourceContains("src/ApplicationDisplayFrontend.h",
        "class DisplayFrontendInitializer");
    assertSourceContains("src/ApplicationDisplayFrontend.cc",
        "LegacyDisplayFrontendInitializer::initializeDisplayFrontend");
    assertSourceContains("src/ApplicationDisplayFrontend.cc", "cth_init(argc, argv)");
    assertSourceContains("src/CMakeLists.txt", "ApplicationDisplayFrontend.cc");
    assertSourceContains("src/Application.cc",
        "displayFrontendInitializer->initializeDisplayFrontend");
    assertSourceDoesNotContain("src/Application.cc", "cth_init");
    assertSourceDoesNotContain("src/Application.cc", "requestApplicationSuspend");
    assertSourceDoesNotContain("src/Application.cc", "CTH_INFO");
    assertSourceDoesNotContain("src/Application.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/Application.cc", "CTH_ERROR");
    assertSourceDoesNotContain("src/Application.cc", "CTH_TRACE");
    assertSourceDoesNotContain("src/Application.cc", "CTH_LOG_ENABLED");
    assertSourceDoesNotContain("src/Application.cc", "cth_log");
    assertSourceDoesNotContain("src/Application.cc", "#include \"cthugha.h\"");
    assertSourceContains("src/PlatformLifecycle.h", "LogSink& log");
    assertSourceContains("src/PlatformLifecycle.h",
        "class PlatformSuspendSignalState");
    assertSourceContains("src/PlatformLifecycle.h",
        "std::unique_ptr<PlatformSuspendSignalState> signalState");
    assertSourceDoesNotContain("src/PlatformLifecycle.h",
        "requestApplicationSuspend");
    assertSourceContains("src/PlatformLifecycle.cc", "PlatformLifecycle::PlatformLifecycle(LogSink& log_");
    assertSourceContains("src/PlatformLifecycle.cc",
        "class PlatformSuspendSignalState");
    assertSourceContains("src/PlatformLifecycle.cc",
        "static PlatformSuspendSignalState* activeSuspendSignalState");
    assertSourceContains("src/PlatformLifecycle.cc",
        "signalState(new PlatformSuspendSignalState)");
    assertSourceContains("src/PlatformLifecycle.cc", "signalState->request()");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc",
        "static volatile sig_atomic_t suspendRequested");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc",
        "static volatile int suspendRequested");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc",
        "static int suspendHandlerInstalled");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc",
        "static struct sigaction previousSuspendAction");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc",
        "requestApplicationSuspend");
    assertSourceContains("src/PlatformLifecycle.cc", "log.warn");
    assertSourceContains("src/PlatformLifecycle.cc", "log.debug");
    assertSourceContains("src/PlatformLifecycle.cc", "log.info");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc", "CTH_INFO");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc", "CTH_WARN");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc", "cth_log");
    assertSourceDoesNotContain("src/PlatformLifecycle.cc", "#include \"cthugha.h\"");
    assertSourceContains("src/ProcessServices.h", "class LoggingRuntime");
    assertSourceContains("src/ProcessServices.h", "#include \"LogLevels.h\"");
    assertSourceContains("src/LogLevels.h", "enum CthughaLogLevel");
    assertSourceContains("src/cthugha.h", "#include \"LogLevels.h\"");
    assertSourceDoesNotContain("src/cthugha.h", "enum CthughaLogLevel");
    assertSourceDoesNotContain("src/ProcessServices.cc", "#include \"cthugha.h\"");
    assertSourceContains("src/ProcessServices.h",
        "void cthugha_install_logging_runtime(LoggingRuntime& runtime)");
    assertSourceContains("src/ProcessServices.h",
        "void cthugha_clear_logging_runtime(LoggingRuntime& runtime)");
    assertSourceContains("src/ProcessServices.cc", "LoggingRuntime::configure");
    assertSourceContains("src/CMakeLists.txt", "LegacyLoggingAdapter.cc");
    assertSourceContains("src/LegacyLoggingAdapter.cc",
        "static LoggingRuntime* cthugha_logging_runtime");
    assertSourceDoesNotExist("src/misc.cc");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "cthugha_logging_verbosity");
    assertSourceDoesNotContain("src/cthugha.h", "cthugha_install_logging_runtime");
    assertSourceDoesNotContain("src/cthugha.h", "cthugha_clear_logging_runtime");
    assertSourceDoesNotContain("src/cthugha.h", "cthugha_configure_logging");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "cthugha_configure_logging");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "cthugha_verbose");
    assertSourceDoesNotContain("src/Option.h", "cthugha_verbose");
}

static void testAudioInputFileGlobalWasDeleted() {
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_input_file");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_input_file");
}

static void testLegacyParserDoesNotWritePortedConfigValues() {
    assertSourceDoesNotExist("src/options.cc");
    assertSourceDoesNotExist("src/options.h");
    assertSourceDoesNotExist("src/xwin_options.cc");
    assertSourceDoesNotContain("src/Application.cc", "get_pre_params");
    assertSourceDoesNotContain("src/Application.cc", "get_params");
    assertSourceDoesNotContain("src/Application.cc", "params_request_help");
    assertSourceDoesNotContain("src/Application.cc", "startupHelpRequested");
    assertSourceContains("src/Application.cc", "startupConfig.helpRequested");
    assertSourceDoesNotContain("src/options.cc", "audioInputMode.setValue");
    assertSourceDoesNotContain("src/options.cc", "audio_input_file");
    assertSourceDoesNotContain("src/options.cc", "display_mode =");
    assertSourceDoesNotContain("src/options.cc",
        "CthughaBuffer::buffer.setDimensions");
    assertSourceDoesNotContain("src/options.cc", "cthugha_verbose.change");
    assertSourceDoesNotContain("src/options.cc", "&cthugha_verbose.value");
    assertSourceDoesNotContain("src/options.cc", "&options_save.value");
    assertSourceDoesNotContain("src/options.cc", "snprintf(extra_lib_path");
    assertSourceDoesNotContain("src/options.cc", "strncpy(ini_file_override");
    assertSourceDoesNotContain("src/options.cc", "&audioInputLoop");
    assertSourceDoesNotContain("src/options.cc", "soundChannels.setValue");
    assertSourceDoesNotContain("src/options.cc", "soundSampleRate.setValue");
    assertSourceDoesNotContain("src/options.cc", "soundFormat.change");
    assertSourceDoesNotContain("src/options.cc", "strncpy(pulse_server");
    assertSourceDoesNotContain("src/options.cc", "pulse_latency_msec =");
    assertSourceDoesNotContain("src/options.cc", "strncpy(audio_output_dump");
    assertSourceDoesNotContain("src/options.cc", "sound_minnoise.change");
    assertSourceDoesNotContain("src/options.cc", "soundSilent.setValue");
    assertSourceDoesNotContain("src/options.cc", "strncpy(dev_dsp");
    assertSourceDoesNotContain("src/options.cc", "soundDSPSync.setValue");
    assertSourceDoesNotContain("src/options.cc", "strncpy(dev_mixer");
    assertSourceDoesNotContain("src/options.cc", "soundDSPFragments.change");
    assertSourceDoesNotContain("src/options.cc", "soundDSPMethod.change");
    assertSourceDoesNotContain("src/options.cc", "mixer_initial_volume");
    assertSourceDoesNotContain("src/options.cc", "&lock.value");
    assertSourceDoesNotContain("src/options.cc", "&change_little.value");
    assertSourceDoesNotContain("src/options.cc", "lock.setValue");
    assertSourceDoesNotContain("src/options.cc", "changeWaitMin.change");
    assertSourceDoesNotContain("src/options.cc", "changeWaitRandom.change");
    assertSourceDoesNotContain("src/options.cc", "changeQuiet.change");
    assertSourceDoesNotContain("src/options.cc", "changeMsgTime.change");
    assertSourceDoesNotContain("src/options.cc",
        "changeCumulativeFireLevel.change");
    assertSourceDoesNotContain("src/options.cc", "silenceMessages().loadFile");
    assertSourceDoesNotContain("src/options.cc", "setQotdEnabled");
    assertSourceDoesNotContain("src/options.cc", "setQotdServer");
    assertSourceDoesNotContain("src/options.cc", "&use_objects.value");
    assertSourceDoesNotContain("src/options.cc", "&use_translates.value");
    assertSourceDoesNotContain("src/options.cc", "&showFPS.value");
    assertSourceDoesNotContain("src/options.cc", "paletteSmoothingChance =");
    assertSourceDoesNotContain("src/options.cc", "palette_set_filter");
    assertSourceDoesNotContain("src/options.cc",
        "videoDirector().setImageLoadingEnabled");
    assertSourceDoesNotContain("src/options.cc", "zoom.change");
    assertSourceDoesNotContain("src/options.cc", "maxFramesPerSecond.change");
    assertSourceDoesNotContain("src/options.cc", "&key_esc");
    assertSourceDoesNotContain("src/options.cc", "Keymap::keymapFile");
    assertSourceContains("src/Mixer.cc",
        "MixerInitialVolume(it->name, it->volume)");
}

static void testCatalogLoadingUsesPathConfig() {
    assertSourceContains("src/EffectChoiceLoader.cc", "pathConfig.extraLibraryPath");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "extra_lib_path");
    assertSourceContains("src/Image.cc", "loadEffectChoices(*this, pathConfig");
    assertSourceContains("src/palettes.cc", "loadEffectChoices(palette, pathConfig");
    assertSourceContains("src/waves.cc", "loadEffectChoices(object, pathConfig");
}

static void testCatalogLoadingSkipsDuplicateNamesWithoutCompressedAssetShellOut() {
    assertSourceContains("src/EffectChoiceLoader.cc", "option.defined(feat_name)");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "double_load");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "isCompressed");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "popen");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "pclose");
    assertSourceDoesNotContain("src/options.cc", "dbl-load");
    assertSourceDoesNotContain("src/Option.h", "double_load");
    assertSourceDoesNotContain("src/Configuration.h", "CatalogConfig");
}

static void testScreenshotFeatureWasRemoved() {
    assertSourceDoesNotContain("src/options.cc", "prt" "-file");
    assertSourceDoesNotContain("src/DisplayDevice.h", "print" "Screen");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "print" "Screen");
    assertSourceDoesNotContain("src/keymap.cc", "print" "Screen");
    assertSourceDoesNotContain("src/Configuration.h", "screenshot" "FilePrefix");
    assertSourceDoesNotContain("src/configuration_defaults.h",
        "SCREENSHOT" "_FILE_PREFIX");
    assertSourceDoesNotContain("src/CMakeLists.txt", "Screen" "shot.cc");
}

static void testLegacyTestOptionWasRemoved() {
    assertSourceDoesNotContain("src/options.cc", "opt_" "test");
    assertSourceDoesNotContain("src/options.cc", "\"test\"");
}

static void testApplicationProvidesStartupConfigSlices() {
    assertSourceContains("src/Application.cc",
        "startupConfigValue.app.optionsSaveEnabled");
    assertSourceContains("src/Application.cc", "configureKeys(startupConfigValue.input)");
    assertSourceContains("src/Application.cc", "Keymap::init(startupConfigValue.input)");
    assertSourceContains("src/Application.cc",
        "remove_continuation_ini(startupConfigValue.paths, logSinkValue)");
    assertSourceContains("src/Application.cc", "initMixerRuntime()");
    assertSourceContains("src/Application.cc", "configureCthughaDisplay(startupConfigValue.display)");
    assertSourceContains("src/Application.cc",
        "new OwnedAutoChangeSettings(startupConfigValue.autoChange)");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.audioAnalysis.minNoise");
    assertSourceContains("src/Configuration.h", "struct AudioAnalysisConfig");
    assertSourceContains("src/Configuration.h", "AudioAnalysisConfig audioAnalysis");
    assertSourceDoesNotContain("src/Application.cc", "configureAudioAnalyzer");
    assertSourceContains("src/Application.cc", "configureEffectPolicy(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureTranslationOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureWaveOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configurePaletteOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "audioProcessingSelectorValue->configureStartup(startupConfigValue.scene)");
    assertSourceContains("src/Application.cc",
        "videoDirector().configureTransitions(startupConfigValue.sceneTransition)");
    assertSourceContains("src/Application.cc",
        "videoDirector().configureQuietMessages(startupConfigValue.messages)");
    assertSourceContains("src/Application.cc", "sceneCommands().applyStartupConfig(startupConfigValue.scene)");
    assertSourceDoesNotContain("src/Application.cc", "Keymap::configure");
    assertSourceDoesNotContain("src/Application.cc", "EffectControl::changeToInitial");
    assertSourceDoesNotContain("src/Application.cc", "audioProcessing.changeToInitial");
    assertSourceDoesNotContain("src/Application.cc", "applyEffectPolicy");
    assertSourceDoesNotContain("src/Application.cc",
        "read_effect_control_usage_and_presets");
    assertSourceDoesNotContain("src/Application.cc", "configureApplicationOptions");
    assertSourceDoesNotContain("src/Option.h", "options_save");
    assertSourceDoesNotContain("src/Option.cc", "options_save");
    assertSourceDoesNotContain("src/Option.h", "configureApplicationOptions");
    assertSourceDoesNotContain("src/Option.cc", "configureApplicationOptions");
    assertSourceDoesNotContain("src/AutoChanger.cc", "configureQuietMessages");
    assertSourceDoesNotContain("src/AutoChanger.h", "MessagesConfig");
    assertSourceDoesNotContain("src/Configuration.h", "AutoChangeConfig {\n"
        "    int quietMs;\n"
        "    int waitMinMs;\n"
        "    int waitRandomMs;\n"
        "    int waitRandomMinimumMs;\n"
        "    int cumulativeFireLevel;\n"
        "    int locked;\n"
        "    int changeLittle;\n"
        "    int minNoise;\n"
        "    int quietMessageMs;");
}

static void testInputStartupUsesInputConfig() {
    assertSourceContains("src/Configuration.h", "struct InputConfig");
    assertSourceContains("src/Configuration.h", "InputConfig input");
    assertSourceContains("src/keys.h", "void configureKeys(const InputConfig& config)");
    assertSourceContains("src/keymap.h", "static void init(const InputConfig& config)");
    assertSourceDoesNotContain("src/Configuration.h",
        "struct AppConfig {\n    int optionsSaveEnabled;\n    int escapeKeyEnabled");
    assertSourceDoesNotContain("src/keymap.h", "keymapFile[PATH_MAX]");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::configure");
}

static void testSceneStartupUsesSceneConfig() {
    assertSourceContains("src/Configuration.h", "SceneConfig scene");
    assertSourceContains("src/Scene.cc", "SceneCommands::applyStartupConfig");
    assertSourceContains("src/Scene.cc", "config.flame");
    assertSourceContains("src/Scene.cc", "config.wave");
    assertSourceContains("src/Scene.cc", "config.palette");
    assertSourceDoesNotContain("src/options.cc", "flame.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "wave.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "waveScale.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "object.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "translation.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "palette.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "border.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "flashlight.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "table.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "imageOption().setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "screen.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "audioProcessing.setInitialEntry");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlGetIniInitials");
}

static void testIniPersistenceUsesRuntimePersistenceAdapter() {
    assertSourceContains("src/IniFiles.h", "struct ContinuationIniConfig");
    assertSourceContains("src/IniFiles.h", "class LogSink");
    assertSourceContains("src/IniFiles.h",
        "int write_ini(const Config& config, LogSink& log)");
    assertSourceContains("src/IniFiles.h",
        "int remove_continuation_ini(const PathConfig& paths, LogSink& log)");
    assertSourceDoesNotContain("src/IniFiles.h", "void configure_ini_persistence");
    assertSourceDoesNotContain("src/IniFiles.h", "int write_ini();");
    assertSourceDoesNotContain("src/IniFiles.h", "getini");
    assertSourceDoesNotContain("src/IniFiles.h", "putini");
    assertSourceContains("src/IniFiles.cc",
        "int write_ini(const Config& config, LogSink& log)");
    assertSourceDoesNotContain("src/IniFiles.cc", "int write_ini()");
    assertSourceDoesNotContain("src/IniFiles.cc", "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/IniFiles.cc", "CTH_DEBUG");
    assertSourceDoesNotContain("src/IniFiles.cc", "CTH_INFO");
    assertSourceDoesNotContain("src/IniFiles.cc", "CTH_ERROR");
    assertSourceDoesNotContain("src/IniFiles.cc", "CTH_ERRNO");
    assertSourceDoesNotContain("src/IniFiles.cc", "cth_log");
    assertSourceDoesNotContain("src/IniFiles.cc", "static FILE*");
    assertSourceDoesNotContain("src/IniFiles.cc", "static std::string fname");
    assertSourceContains("src/IniFiles.cc",
        "static std::string home_ini_file_name");
    assertSourceDoesNotContain("src/IniFiles.cc", "persisted_startup_config");
    assertSourceDoesNotContain("src/IniFiles.cc", "has_persisted_startup_config");
    assertSourceContains("src/IniFiles.cc",
        "int write_continuation_ini(const ContinuationIniConfig& config, LogSink& log)");
    assertSourceContains("src/IniFiles.cc", "class IniWriter");
    assertSourceContains("src/IniFiles.cc", "log.errorErrno");
    assertSourceContains("src/IniFiles.cc", "log.info");
    assertSourceContains("src/IniFiles.cc",
        "write_scene_config_ini(writer, config.scene)");
    assertSourceContains("src/IniFiles.cc",
        "write_audio_analysis_config_ini(writer, config.audioAnalysis)");
    assertSourceContains("src/IniFiles.cc",
        "write_auto_change_config_ini(writer, config.autoChange)");
    assertSourceContains("src/IniFiles.cc",
        "write_effect_policy_ini(writer, config.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "RuntimeConfigRegistry(startupConfigValue)");
    assertSourceContains("src/Application.cc",
        "new IniRuntimePersistence(*runtimeConfigRegistryValue, logSinkValue)");
    assertSourceContains("src/Application.cc",
        "runtimePersistenceValue->writeCurrentConfig()");
    assertSourceDoesNotContain("src/Application.cc", "configure_ini_persistence");
    assertSourceDoesNotContain("src/Application.cc", "write_ini(startupConfigValue)");
    assertSourceDoesNotContain("src/Application.cc",
        "write_ini(runtimeConfigRegistryValue->currentConfig())");
    assertSourceContains("src/RuntimeChangeMediator.h",
        "RuntimePersistence& runtimePersistence_");
    assertSourceContains("src/RuntimeChangeMediator.h",
        "RuntimeShutdown& runtimeShutdown_");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimePersistence.writeCurrentConfig()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "IniFiles.h");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "RuntimeConfigRegistry.h");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "write_ini()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "write_ini(");
    assertSourceContains("src/keymap.cc",
        "RuntimeCommand::stopAndContinue");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimePersistence.writeContinuation()");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "RuntimeContinuationState");
    assertSourceDoesNotContain("src/keymap.cc",
        "current_continuation_state");
    assertSourceDoesNotContain("src/keymap.cc",
        "Keymap::audioProcessingState()");
    assertSourceDoesNotContain("src/keymap.cc",
        "sceneCommandsForLegacyCallbacks()");
    assertSourceDoesNotContain("src/keymap.cc",
        "screen.currentName()");
    assertSourceDoesNotContain("src/Application.cc",
        "Keymap::setAudioProcessingState");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "write_continuation_ini(");
    assertSourceContains("src/RuntimePersistence.cc",
        "write_ini(runtimeConfigRegistry.currentConfig(), log)");
    assertSourceContains("src/RuntimePersistence.cc",
        "Config current = runtimeConfigRegistry.currentConfig()");
    assertSourceContains("src/RuntimePersistence.cc",
        "continuationIniConfigFromConfig(current), log)");
    assertSourceContains("src/RuntimePersistence.h", "LogSink& log");
    assertSourceContains("tests/unit/RuntimePersistenceTest.cc",
        "assert(lastIniLogSink == &testLogSink)");
    assertSourceContains("tests/unit/RuntimePersistenceTest.cc",
        "assert(lastContinuationLogSink == &testLogSink)");
    assertSourceContains("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessingState.text()");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessing.text()");
    assertSourceDoesNotContain("src/keymap.cc",
        "audioProcessing.text()");
    assertSourceDoesNotContain("src/AutoChanger.cc", "write_ini");
    assertSourceDoesNotContain("src/AutoChanger.cc", "options_save");
    assertSourceDoesNotExist("src/EffectControlIni.cc");
    assertSourceDoesNotExist("src/EffectControlIni.h");
    assertSourceDoesNotContain("src/CMakeLists.txt", "EffectControlIni.cc");
    assertSourceDoesNotContain("src/IniFiles.cc", "int getini");
    assertSourceDoesNotContain("src/IniFiles.cc", "move_ini_file");
    assertSourceDoesNotContain("src/IniFiles.cc", "is_in_ini");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(audioProcessing)");
    assertSourceDoesNotContain("src/IniFiles.cc", "showFPS");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeWait");
    assertSourceDoesNotContain("src/IniFiles.cc",
        "putini(changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeQuiet");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeMsgTime");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(lock)");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(change_little)");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlPutIniUsages");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlPutPresetIni");
}

static void testRuntimeLifecycleRequestsUseMediator() {
    assertSourceContains("src/keymap.cc", "RuntimeCommand::requestClose()");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::requestClose()");
    assertSourceContains("src/InterfaceCredits.cc",
        "RuntimeCommand::requestClose()");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc", "cthugha_close++");
    assertSourceDoesNotContain("src/InterfaceCredits.cc", "cthugha_close++");
    assertSourceDoesNotExist("src/AudioRuntime.cc");
    assertSourceContains("src/Application.cc", "new RuntimeCloseState()");
    assertSourceContains("src/Application.cc",
        "runtimeShutdownValue->requestClose()");
    assertSourceContains("src/Application.cc", "closeRequested()");
    assertSourceDoesNotContain("src/Application.cc", "cthugha_close++");
    assertSourceDoesNotContain("src/Application.cc", "cthugha_close == 0");
    assertSourceDoesNotContain("src/cthugha.h", "cthugha_close");
    assertSourceDoesNotExist("src/misc.cc");
    assertSourceDoesNotContain("src/cthugha.h", "gettime()");
    assertSourceDoesNotContain("src/cthugha.h", "getTime()");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "gettime()");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "getTime()");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimeShutdown.requestClose()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "cthugha_close++");
    assertSourceContains("src/RuntimeShutdown.cc", "closeRequestedValue");
    assertSourceDoesNotContain("src/RuntimeShutdown.cc", "cthugha_close++");
    assertSourceContains("src/Application.h", "FramePacer framePacerValue");
    assertSourceContains("src/Application.h", "SystemSecondsClock secondsClockValue");
    assertSourceContains("src/Application.h", "SystemMillisecondClock millisecondClockValue");
    assertSourceContains("src/Application.cc", "secondsClockValue.nowSeconds()");
    assertSourceContains("src/Application.cc",
        "new InterfaceRuntime(millisecondClockValue)");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.display, secondsClockValue");
    assertSourceContains("src/Application.cc",
        "displayRuntimeOwnership->runtime(), secondsClockValue");
    assertSourceDoesNotContain("src/Application.cc", "getTime()");
    assertSourceContains("src/InterfaceRuntime.h", "MillisecondClock& clock");
    assertSourceContains("src/InterfaceRuntime.cc", "clock.milliseconds()");
    assertSourceDoesNotContain("src/Interface.cc", "gettime()");
    assertSourceDoesNotContain("src/InterfaceCredits.cc", "gettime()");
    assertSourceContains("src/FrameClock.h", "SecondsClock& clock");
    assertSourceDoesNotContain("src/FrameClock.h", "FrameTimeSource");
    assertSourceDoesNotContain("src/FrameClock.h", "SystemFrameTimeSource");
    assertSourceDoesNotContain("src/FrameClock.cc", "getTime()");
    assertSourceContains("src/DisplayDeviceX11.cc", "clock.nowSeconds()");
    assertSourceContains("src/CthughaDisplayX11.cc", "clock.nowSeconds()");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "getTime()");
    assertSourceDoesNotContain("src/CthughaDisplayX11.cc", "getTime()");
    assertSourceContains("src/CthughaDisplay.h", "FrameClock frameClockValue");
    assertSourceContains("src/CthughaDisplay.h", "double frameNowValue");
    assertSourceContains("src/CthughaDisplay.h", "double currentFrameTimeSeconds() const");
    assertSourceContains("src/Application.cc",
        "displayValue->currentFrameTimeSeconds()");
    assertSourceDoesNotContain("src/CthughaDisplay.h", "extern double now");
    assertSourceDoesNotContain("src/CthughaDisplay.h", "extern double deltaT");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "double now =");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "double deltaT =");
    assertSourceDoesNotContain("src/CthughaDisplay.cc",
        "static SystemSecondsClock displaySecondsClock");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "static FrameClock");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "frameClock.publish");
    assertSourceDoesNotContain("src/FrameClock.h", "publish(double&");
    assertSourceDoesNotContain("src/Application.cc", "visualFrameStart = now");
    assertSourceDoesNotContain("src/Application.cc", "context.now = now");
    assertSourceDoesNotContain("src/Application.cc", "static FramePacer");
    assertSourceDoesNotContain("src/Application.cc", "static SystemFrameSleeper");
    assertSourceDoesNotContain("src/cthugha.h", "systemf");
    assertSourceDoesNotContain("src/LegacyLoggingAdapter.cc", "systemf");
}

static void testRuntimeCommandsUseSubsystemControlPorts() {
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeDisplayControls(randomSourceValue)");
    assertSourceContains("src/RuntimeDisplayControls.h",
        "RandomSource& randomSource");
    assertSourceContains("src/RuntimeDisplayControls.cc",
        "screen.change(to, randomSource, 0)");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAudioControls(*audioProcessingSelectorValue,");
    assertSourceContains("src/Application.cc",
        "mixerControlsValue.get())");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue)");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeEffectControls(randomSourceValue)");
    assertSourceContains("src/RuntimeEffectControls.h",
        "RandomSource& randomSource");
    assertSourceContains("src/RuntimeEffectControls.cc",
        "control.change(to, randomSource, 0)");
    assertSourceContains("tests/unit/EffectControlRandomTest.cc",
        "testRuntimeEffectControlsUseInjectedRandomSource");
    assertSourceDoesNotContain("src/Application.cc",
        "new DefaultRuntimeOptionControls()");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeDisplayControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeAudioControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeAutoChangeControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeEffectControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeCommandTargets.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeOptionControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "displayControls.changePresentationBy(command.value)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "audioControls.changeSoundProcessingBy(command.value)");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "RuntimeCommandResetAudioFrame");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "resetAudioFrame");
    assertSourceDoesNotContain("src/RuntimeCommandSink.h",
        "audioResetRequested");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "RuntimeCommandResetAudioFrame");
    assertSourceDoesNotContain("src/keymap.cc", "soundReset");
    assertSourceDoesNotContain("src/default.keymap", "soundReset");
    assertSourceDoesNotContain("src/keymap.cc",
        "#include \"AudioAnalyzer.h\"");
    assertSourceDoesNotContain("src/keymap.cc",
        "#include \"AudioFrame.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "autoChangeControls.toggleLock()");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "command.optionTarget->changeBy(command.value)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "command.effectControlTarget->changeBy(command.value)");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "RoutedRuntimeOptionTarget target");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "RoutedRuntimeEffectControlTarget target");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.h",
        "changeOwnedOptionBy(Option& option");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.h",
        "applyNonSceneEffectControlBy(EffectControl& control");
    assertSourceContains("src/RuntimeCommand.h", "class RuntimeOptionTarget");
    assertSourceContains("src/RuntimeCommand.h",
        "RuntimeOptionTarget* optionTarget");
    assertSourceContains("src/RuntimeCommand.h",
        "RuntimeEffectControlTarget* effectControlTarget");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "Option* option");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "EffectControl* effectControl");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RuntimeOptionTarget");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RoutedRuntimeOptionTarget");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RuntimeEffectControlTarget");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RoutedRuntimeEffectControlTarget");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RuntimeCommandTargetRouter");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RoutedRuntimeCommandTargetRouter");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "displayControls.changeDisplayOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "audioControls.changeAudioOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "autoChangeControls.changeAutoChangeOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "effectControls.changeEffectControlBy(control, by)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "effectControls.toggleEffectChoiceUse(control, index)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "RuntimeCommand::changeOptionBy(target, by)");
    assertSourceContains("src/RuntimeCommandTargets.cc",
        "RuntimeCommand::activateEffectControl(target, index)");
    assertSourceContains("tests/unit/RuntimeChangeMediatorTest.cc",
        "testGenericCommandsCanRouteThroughExplicitTargets");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<RuntimeCommandTargetRouter> runtimeCommandRouterValue");
    assertSourceContains("src/Application.cc",
        "new RoutedRuntimeCommandTargetRouter(");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setCommandRouter(runtimeCommandRouterValue.get())");
    assertSourceContains("src/InterfaceRuntime.cc",
        "commandRouterValue->toggleEffectChoiceUse");
    assertSourceContains("src/InterfaceRuntime.cc",
        "commandRouterValue->changeOptionBy");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "runtimeCommandRouter.activateEffectControl");
    assertSourceDoesNotContain("src/InterfaceRuntime.cc",
        "legacyRuntimeCommand");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc",
        "legacyRuntimeCommand");
    assertSourceDoesNotContain("src/RuntimeCommandTargets.h",
        "legacyRuntimeCommand");
    assertSourceDoesNotContain("src/RuntimeCommandTargets.cc",
        "legacyRuntimeCommand");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "changeOptionBy(Option&");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "changeEffectControlBy(EffectControl&");
    assertSourceContains("tests/CMakeLists.txt",
        "RuntimeCommandTargets.cc");
    assertSourceContains("src/InterfaceList.cc",
        "runtime->toggleContextEffectChoiceUse()");
    assertSourceDoesNotExist("src/RuntimeOptionControls.cc");
    assertSourceDoesNotExist("src/RuntimeOptionControls.h");
    assertSourceDoesNotExist("src/RuntimeEffectCatalogControls.cc");
    assertSourceDoesNotExist("src/RuntimeEffectCatalogControls.h");
    assertSourceDoesNotContain("src/CMakeLists.txt",
        "RuntimeOptionControls.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt",
        "RuntimeEffectCatalogControls.cc");
    assertSourceContains("src/CMakeLists.txt",
        "RuntimeEffectControls.cc");
    assertSourceContains("src/CMakeLists.txt",
        "RuntimeCommandTargets.cc");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "#include \"AudioAnalyzer.h\"");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "#include \"AudioFrame.h\"");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "sound_minnoise");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "audioProcessingSelector.changeBy");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "audioProcessingSelector.changeTo");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "mixerControls->changeOptionBy(option, by)");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "mixerControls->changeOptionTo(option, to)");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AudioFrame.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AudioProcessor.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AutoChanger.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"CthughaDisplay.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"EffectControl.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"Option.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"Screen.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "audioProcessing.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "audioFrameChange");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "lock.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "showFPS.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "screen.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "zoom.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "&screen");
}

static void testX11PanelInputsUseRuntimeCommands() {
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::savePaletteMetadata");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::revertPaletteMetadata");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc",
        "d->opt->setValue");
}

static void testSelectionDisplaysUseRuntimeConfigRegistry() {
    assertSourceContains("src/Interface.cc",
        "registry->currentConfig()");
    assertSourceContains("src/Interface.cc",
        "runtimeConfigSelectionTextOrFallback");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "runtimeConfigRegistry.currentConfig()");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "updatePanelSelectionLabels()");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setRuntimeConfigRegistry(runtimeConfigRegistryValue.get())");
    assertSourceContains("src/Application.cc",
        "*runtimeConfigRegistryValue,\n        startupConfigValue.display");
}

static void testX11PanelUpdatesOnlyWhenStateChanges() {
    assertSourceContains("src/DisplayDeviceX11.cc",
        "panelPendingTextSignature != panelCommittedTextSignature");
    assertSourceContains("src/DisplayDeviceX11.cc", "panelTextDirty");
    assertSourceContains("src/DisplayDeviceX11.cc", "markPanelTextCopyRect");
    assertSourceContains("src/DisplayDeviceX11.cc", "panelTextCopyWidth");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "strcmp(panelMenuLabels[i], nextLabel) == 0");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "panelSelectionSignature");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "palettePreviewFingerprintValid");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "drawPalettePreviewRect");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc", "panelTextExpose");
}

static void testInterfaceInputsDoNotUseLegacyFallbacks() {
    assertSourceDoesNotContain("src/Interface.cc",
        "currentEffectControl->change");
    assertSourceDoesNotContain("src/Interface.cc",
        "currentOption->change");
    assertSourceDoesNotContain("src/Interface.cc",
        "currentEffectControl->lock.change");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "currentOption->change");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "currentEffectControl->setValue");
    assertSourceDoesNotContain("src/Interface.cc", "currentOption");
    assertSourceDoesNotContain("src/Interface.cc", "currentEffectControl");
    assertSourceDoesNotContain("src/Interface.cc",
        "currentOptionInterfaceElement");
    assertSourceDoesNotContain("src/InterfaceList.cc", "currentOption");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "currentEffectControl");
}

static void testRemainingSharedRuntimeStateWasRemoved() {
    assertSourceDoesNotContain("src/AudioPulseOutput.cc",
        "audioPulseUnderflowCount");
    assertSourceContains("src/Audio.h",
        "std::atomic<int> underflowCountValue");
    assertSourceContains("src/AudioFramePipeline.h",
        "int debugReportsValue");
    assertSourceDoesNotContain("src/AudioInternal.cc", "static int reports");
    assertSourceContains("src/Audio.h",
        "class AudioSubmittedPcmDebugReporter");
    assertSourceDoesNotContain("src/AudioInternal.h",
        "audioDebugSubmittedPcm");

    assertSourceContains("src/InterfaceRuntime.h", "class InterfaceRuntime");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<InterfaceRuntime> interfaceRuntimeValue");
    assertSourceContains("src/Application.cc",
        "registerDefaultInterfaces(*interfaceRuntimeValue)");
    assertSourceContains("src/keymap.cc", "Keymap::setInterfaceRuntime");
    assertSourceContains("src/CthughaDisplayX11.cc",
        "runtime->current()");

    assertSourceDoesNotContain("src/Interface.h", "Interface::current");
    assertSourceDoesNotContain("src/Interface.h", "Interface::head");
    assertSourceDoesNotContain("src/Interface.h", "static Interface*");
    assertSourceDoesNotContain("src/Interface.h", "saveToPreset");
    assertSourceDoesNotContain("src/Interface.h", "showStatus");
    assertSourceDoesNotContain("src/Interface.cc", "Interface::current");
    assertSourceDoesNotContain("src/Interface.cc", "Interface::head");
    assertSourceDoesNotContain("src/Interface.cc",
        "runtimeConfigRegistryValue =");
    assertSourceDoesNotContain("src/Interface.cc",
        "autoChangerStatusProviderValue =");
    assertSourceDoesNotContain("src/Interface.cc",
        "autoChangeControlsValue =");
    assertSourceDoesNotContain("src/Interface.cc",
        "audioProcessingSelectorValue =");
    assertSourceDoesNotContain("src/CthughaDisplayX11.cc",
        "Interface::current");
}

static void testTranslationGenerationUsesApplicationRandomSource() {
    assertSourceContains("src/Application.cc",
        "initializeVisualCatalogs(CthughaBuffer::buffer, startupConfigValue.paths,\n"
        "            randomSourceValue)");
    assertSourceContains("src/Application.cc", "init_translate(buffer, randomSource)");
    assertSourceContains("src/TranslationOptions.h",
        "int init_translate(const CthughaBuffer& buffer, RandomSource& randomSource)");
    assertSourceContains("src/TranslationOptions.cc",
        "defaultTranslationCatalog().generateAll(buffer.width(), buffer.height(),\n"
        "            randomSource, generatedTables)");
    assertSourceContains("src/TranslateGenerator.h",
        "void generateAll(int width, int height, RandomSource& randomSource");
    assertSourceDoesNotContain("src/TranslateGenerator.cc",
        "high_resolution_clock");
    assertSourceDoesNotContain("src/TranslateGenerator.cc", "#include <chrono>");
    assertSourceContains("tests/unit/TranslateGeneratorTest.cc",
        "testCatalogRandomizesSeedsFromInjectedRandomSource");
}

static void testPaletteTransitionUsesInjectedRandomSource() {
    assertSourceContains("src/PaletteTransition.h",
        "randomPaletteTransitionStrategy(\n"
        "    RandomSource& randomSource)");
    assertSourceContains("src/PaletteTransition.cc",
        "randomSource.uniformInt(nPaletteTransitionStrategies)");
    assertSourceDoesNotContain("src/PaletteTransition.cc", "rand()");
    assertSourceContains("src/VideoDirector.cc",
        "randomPaletteTransitionStrategy(*randomSourceValue)");
    assertSourceContains("tests/unit/PaletteTransitionTest.cc",
        "testRandomStrategyUsesInjectedRandomSource");
}

static void testPaletteGenerationUsesInjectedRandomSource() {
    assertSourceContains("src/PaletteRandomGenerator.h",
        "generateRandomPalette(ColorPalette& palette, RandomSource& randomSource)");
    assertSourceContains("src/PaletteRandomGenerator.cc",
        "randomSource.uniformInt(3)");
    assertSourceContains("src/PaletteRandomGenerator.cc",
        "randomSource.uniformInt(256)");
    assertSourceContains("src/display.h",
        "static void addRandom(RandomSource& randomSource)");
    assertSourceContains("src/display.h",
        "static void randomizeLast(RandomSource& randomSource)");
    assertSourceContains("src/display.h",
        "Re-randomizes the currently selected random.N palette");
    assertSourceContains("src/palettes.cc",
        "generateRandomPalette(colors(), randomSource)");
    assertSourceContains("src/palettes.cc", "maxLoadedRandomPaletteIndex()");
    assertSourceContains("src/palettes.cc", "maxPersistedRandomPaletteIndex()");
    assertSourceContains("src/palettes.cc", "randomPaletteOutputDirectory");
    assertSourceContains("src/palettes.cc", "palette.currentPaletteEntry()");
    assertSourceDoesNotContain("src/palettes.cc", "#include \"imath.h\"");
    assertSourceDoesNotContain("src/palettes.cc", "::Random(3)");
    assertSourceDoesNotContain("src/palettes.cc", "::Random(256)");
    assertSourceDoesNotContain("src/display.h", "static void Random()");
    assertSourceContains("src/Scene.cc",
        "PaletteEntry::randomizeLast(randomSource)");
    assertSourceContains("src/Scene.cc",
        "PaletteEntry::addRandom(randomSource)");
    assertSourceContains("tests/unit/PaletteRandomGeneratorTest.cc",
        "testRandomPaletteUsesInjectedRandomSource");
    assertSourceContains("tests/CMakeLists.txt", "palette_random_generator_test");
    assertSourceContains("tests/CMakeLists.txt", "random_palette_persistence_test");
    assertSourceContains("tests/unit/RandomPalettePersistenceTest.cc",
        "testRandomPaletteNamesContinueAfterPersistedFilesAndLoadedEntries");
}

static void testWavesUseInjectedRandomSource() {
    assertSourceContains("src/Wave.h", "RandomSource& randomSource");
    assertSourceContains("src/Wave.h", "int randomInt(int exclusiveMax)");
    assertSourceContains("src/Wave.h", "int randomCenteredInt(int magnitude)");
    assertSourceContains("src/Wave.h", "double randomUnit()");
    assertSourceContains("src/Wave.cc",
        "lookupTables, randomSource, fireBudget)");
    assertSourceContains("src/VideoFilters.h",
        "void setRandomSource(RandomSource& randomSource)");
    assertSourceContains("src/VideoFilters.cc",
        "wave->execute(frame.buffer(), frame.context(), config,\n"
        "            needsConfiguration, state, lookupTables, *randomSourceValue)");
    assertSourceContains("src/VideoDirector.cc",
        "waveFilter->setRandomSource(*randomSourceValue)");
    assertSourceContains("src/waves.cc", "runtime.randomInt(256)");
    assertSourceContains("src/waves.cc", "runtime.randomCenteredInt(100)");
    assertSourceDoesNotContain("src/waves.cc", "rand()");
    assertSourceDoesNotContain("src/waves.cc", "rand() %");
    assertSourceDoesNotContain("src/waves.cc", "Random(");
    assertSourceContains("tests/unit/WaveRuntimeRandomTest.cc",
        "testWaveRuntimeRandomUsesInjectedSource");
    assertSourceContains("tests/CMakeLists.txt", "wave_runtime_random_test");
}

static void testImagePlacementUsesInjectedRandomSource() {
    assertSourceContains("src/Image.h",
        "int bufferHeight, RandomSource& randomSource) const");
    assertSourceContains("src/Image.cc",
        "randomSource.uniformInt(imageSize - bufferSize + 1)");
    assertSourceContains("src/Image.cc",
        "randomSource.uniformInt(bufferSize - imageSize + 1)");
    assertSourceDoesNotContain("src/Image.cc", "Random(");
    assertSourceContains("src/VideoDirector.cc",
        "buffer->width(), buffer->height(), *randomSourceValue)");
    assertSourceContains("tests/unit/ImagePlacementTest.cc",
        "testSmallImagePlacementUsesInjectedRandomSource");
}

static void testGeneralFlameUsesInjectedRandomSource() {
    assertSourceContains("src/Scene.h", "RandomSource& randomSource");
    assertSourceContains("src/Application.cc",
        "videoDirector().imageOption(), randomSourceValue)");
    assertSourceContains("src/Scene.cc",
        "flameGeneral.changeRandom(randomSource)");
    assertSourceContains("src/Scene.cc",
        "applyStartupChoice(flameGeneral, config.generalFlame, randomSource)");
    assertSourceContains("src/flames.h",
        "void changeRandom(RandomSource& randomSource, int doSave = 1)");
    assertSourceContains("src/flames.cc",
        "randomSource.uniformInt(generalFlameStates)");
    assertSourceDoesNotContain("src/flames.cc", "#include \"imath.h\"");
    assertSourceDoesNotContain("src/flames.cc", "value = Random");
    assertSourceContains("tests/unit/GeneralFlameOptionTest.cc",
        "testGeneralFlameRandomUsesInjectedRandomSource");
}

static void testEffectControlUsesInjectedRandomSource() {
    assertSourceContains("src/EffectControl.h",
        "virtual void change(const char* to, RandomSource& randomSource");
    assertSourceContains("src/EffectControl.h",
        "virtual void changeRandom(RandomSource& randomSource");
    assertSourceContains("src/EffectControl.h",
        "int optNr(const char* n, RandomSource& randomSource)");
    assertSourceContains("src/EffectControl.cc",
        "value = randomSource.uniformInt(getNEntries())");
    assertSourceContains("src/EffectControl.cc",
        "int n = randomSource.uniformInt(nCandidates)");
    assertSourceContains("src/EffectControl.cc",
        "EffectControl* EffectControl::changeOne(RandomSource& randomSource)");
    assertSourceContains("src/EffectControl.cc",
        "void EffectControl::changeAll(RandomSource& randomSource)");
    assertSourceDoesNotContain("src/EffectControl.cc", "#include \"imath.h\"");
    assertSourceDoesNotContain("src/EffectControl.cc", "rand()");
    assertSourceDoesNotContain("src/EffectControl.cc", "value = Random");
    assertSourceDoesNotContain("src/EffectControl.cc", "return Random");
    assertSourceContains("src/Scene.cc", "option.change(to, randomSource, doSave)");
    assertSourceContains("src/Scene.cc", "EffectControl::changeAll(randomSource)");
    assertSourceContains("src/Scene.cc", "EffectControl::changeOne(randomSource)");
    assertSourceContains("tests/unit/EffectControlRandomTest.cc",
        "testStaticRandomChangesUseInjectedRandomSource");
    assertSourceContains("tests/CMakeLists.txt", "effect_control_random_test");
}

static void testQotdTimingUsesApplicationCountdownTimers() {
    assertSourceContains("src/ProcessServices.h", "class CountdownTimer");
    assertSourceContains("src/ProcessServices.h", "class CountdownTimerFactory");
    assertSourceContains("src/ProcessServices.h", "class SystemCountdownTimerFactory");
    assertSourceContains("src/Application.h",
        "SystemCountdownTimerFactory countdownTimerFactoryValue");
    assertSourceContains("src/Application.cc",
        "videoDirector().setTimerFactory(countdownTimerFactoryValue)");
    assertSourceContains("src/VideoDirector.h",
        "void setTimerFactory(CountdownTimerFactory& timerFactory)");
    assertSourceContains("src/SilenceMessage.h",
        "void setTimerFactory(CountdownTimerFactory& timerFactory)");
    assertSourceContains("src/QotdMessagesProvider.h",
        "void setTimerFactory(CountdownTimerFactory& timerFactory)");
    assertSourceContains("src/QotdMessagesProvider.cc",
        "timer = currentState->timerFactory->startTimer(prefetchTimeoutMs)");
    assertSourceContains("src/QotdMessagesProvider.cc",
        "std::unique_ptr<CountdownTimer> timer");
    assertSourceContains("src/QotdMessagesProvider.cc",
        "timer.millisecondsRemaining()");
    assertSourceDoesNotContain("src/QotdMessagesProvider.cc", "#include <chrono>");
    assertSourceDoesNotContain("src/QotdMessagesProvider.cc",
        "std::chrono::steady_clock::now");
    assertSourceContains("tests/unit/ProcessServicesTest.cc",
        "testSystemCountdownTimerFactoryCreatesTimer");
}

static void testConfigDefaultsAreNotConsumedAsLegacyDefaults() {
    assertSourceContains("src/configuration_defaults.h", "AUDIO_CONFIG_DEFAULT_INPUT_MODE");
    assertSourceDoesNotContain("src/AudioSystem.cc", "DEFAULT_AUDIO_INPUT_MODE");
    assertSourceDoesNotContain("src/AudioSystem.cc", "DEFAULT_SOUND_SAMPLE_RATE_HZ");
    assertSourceDoesNotContain("src/AudioSettings.cc", "DEFAULT_SOUND_DSP_METHOD");
    assertSourceDoesNotContain("src/AudioOutput.cc", "DEFAULT_PULSE_LATENCY_MS");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "DEFAULT_SOUND_MINNOISE");
    assertSourceDoesNotContain("src/AutoChanger.cc", "DEFAULT_CHANGE_QUIET_MS");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "DEFAULT_MAX_FRAMES_PER_SECOND");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "DEFAULT_DISPLAY_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "DEFAULT_X11_MIT_SHM");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "DEFAULT_DOUBLE_LOAD_ENABLED");
    assertSourceDoesNotContain("src/TranslationOptions.cc", "DEFAULT_USE_TRANSLATES_ENABLED");
    assertSourceDoesNotContain("src/waves.cc", "DEFAULT_USE_OBJECTS_ENABLED");
    assertSourceDoesNotContain("src/keymap.cc", "DEFAULT_KEYMAP_FILE_PATH");
    assertSourceDoesNotContain("src/keys.cc", "DEFAULT_ESCAPE_KEY_ENABLED");
    assertSourceContains("src/configuration_defaults.h", "INPUT_CONFIG_DEFAULT_KEYMAP_FILE_PATH");
    assertSourceContains("src/configuration_defaults.h", "INPUT_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED");
}

int main() {
    testAudioIngestUsesAudioConfig();
    testAudioDeviceSettingsAreStartupOnly();
    testAudioOutputSettingsAreSessionOwned();
    testAudioFrameOwnsPerFrameMetrics();
    testDisplayStartupUsesDisplayConfig();
    testX11StartupUsesX11ConfigOnlyForX11Builds();
    testLoggingUsesLoggingConfig();
    testAudioInputFileGlobalWasDeleted();
    testLegacyParserDoesNotWritePortedConfigValues();
    testCatalogLoadingUsesPathConfig();
    testCatalogLoadingSkipsDuplicateNamesWithoutCompressedAssetShellOut();
    testScreenshotFeatureWasRemoved();
    testLegacyTestOptionWasRemoved();
    testApplicationProvidesStartupConfigSlices();
    testAutoChangeSettingsAreApplicationOwned();
    testInputStartupUsesInputConfig();
    testSceneStartupUsesSceneConfig();
    testIniPersistenceUsesRuntimePersistenceAdapter();
    testRuntimeLifecycleRequestsUseMediator();
    testRuntimeCommandsUseSubsystemControlPorts();
    testX11PanelInputsUseRuntimeCommands();
    testSelectionDisplaysUseRuntimeConfigRegistry();
    testX11PanelUpdatesOnlyWhenStateChanges();
    testInterfaceInputsDoNotUseLegacyFallbacks();
    testRemainingSharedRuntimeStateWasRemoved();
    testTranslationGenerationUsesApplicationRandomSource();
    testPaletteTransitionUsesInjectedRandomSource();
    testPaletteGenerationUsesInjectedRandomSource();
    testWavesUseInjectedRandomSource();
    testImagePlacementUsesInjectedRandomSource();
    testGeneralFlameUsesInjectedRandomSource();
    testEffectControlUsesInjectedRandomSource();
    testQotdTimingUsesApplicationCountdownTimers();
    testConfigDefaultsAreNotConsumedAsLegacyDefaults();
    return 0;
}
