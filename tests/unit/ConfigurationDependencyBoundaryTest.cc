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
        "interfaceRuntimeValue->find(\"Mixer\")");
    assertSourceContains("src/Application.cc",
        "mixerControlsValue->installInto(*mixerInterface)");
    assertSourceDoesNotContain("src/Application.cc",
        "installInto(interfaceMixer)");
    assertSourceDoesNotContain("src/Application.cc",
        "clearInterface(interfaceMixer)");
    assertSourceDoesNotContain("src/Interface.h",
        "extern Interface interfaceMixer");
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
        "frameGeneratorValue.geometry().maxDimension(), randomSourceValue");
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
    assertSourceContains("src/SceneChangeScheduler.cc",
        "void SceneChangeScheduler::operator()(const AudioMetrics& metrics)");
    assertSourceContains("src/SceneChangeScheduler.h",
        "AcousticContext& acousticContext_");
    assertSourceContains("src/SceneChangeScheduler.h",
        "class AutoChangeQuietObserver");
    assertSourceContains("src/SceneChangeScheduler.h",
        "AutoChangeQuietObserver& quietObserver_");
    assertSourceContains("src/SceneChangeScheduler.h", "LogSink& log_");
    assertSourceContains("src/SceneChangeScheduler.cc", "log.debug");
    assertSourceContains("src/SceneChangeScheduler.cc",
        "quietObserver.observeQuiet(quiet_length)");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "videoDirector()");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "#include \"VideoDirector.h\"");
    assertSourceContains("src/SceneChangeScheduler.h",
        "mutable char statusTextValue[512]");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "static char txt");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "CTH_DEBUG");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<SceneChangeScheduler> sceneChangeSchedulerValue");
    assertSourceContains("src/Application.cc",
        "sceneChangeSchedulerValue.reset(new SceneChangeScheduler");
    assertSourceContains("src/Application.cc",
        "(*sceneChangeSchedulerValue)(frame.metrics)");
    assertSourceContains("src/SceneChangeScheduler.h",
        "class SceneChangeScheduler : public SceneChangeStatusProvider");
    assertSourceContains("src/SceneChangeScheduler.h",
        "SceneCommandTarget& sceneCommands");
    assertSourceContains("src/Scene.h", "enum SceneSelectionTarget");
    assertSourceContains("src/Scene.h",
        "void change(SceneSelectionTarget target, int by)");
    assertSourceContains("src/RuntimeChangeMediator.h",
        "SceneCommandTarget& sceneCommands");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.h",
        "SceneCommands& sceneCommands");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "sceneSelectionTargetFromRuntime(target)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "sceneCommands.change(sceneSelectionTargetFromRuntime(target), by)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "sceneCommands.change(sceneSelectionTargetFromRuntime(target), to)");
    assertSourceContains("src/Scene.h",
        "class SceneCommandsTarget : public SceneCommandTarget");
    assertSourceContains("src/LegacySceneEffectControlTarget.h",
        "class SceneCommandsEffectControlOwner : public RuntimeEffectControlOwner");
    assertSourceContains("src/LegacySceneEffectControlTarget.h",
        "virtual int ownsEffectControl(const EffectControl& option) const");
    assertSourceDoesNotContain("src/LegacySceneEffectControlTarget.h",
        "class SceneEffectControlTarget");
    assertSourceDoesNotContain("src/LegacySceneEffectControlTarget.h",
        "SceneCommandsEffectControlTarget");
    assertSourceDoesNotContain("src/Scene.h",
        "class SceneEffectControlTarget");
    assertSourceDoesNotContain("src/Scene.h",
        "class SceneCommandsEffectControlTarget : public SceneEffectControlTarget");
    assertSourceDoesNotContain("src/Scene.h",
        "friend class SceneCommandsEffectControlTarget");
    assertSourceDoesNotContain("src/Scene.h",
        "friend class SceneCommandsEffectControlOwner");
    assertSourceDoesNotContain("src/Scene.h",
        "EffectControl& option");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "#include \"LegacySceneEffectControlTarget.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "#include \"LegacySceneEffectControlCatalog.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "#include \"EffectControlPolicy.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "#include \"EffectRegistry.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "#include \"EffectPresetCatalog.h\"");
    assertSourceContains("src/SceneRuntime.h",
        "std::unique_ptr<RuntimeEffectControlOwner> effectControlOwnerValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "SceneEffectControlTarget");
    assertSourceContains("src/SceneRuntime.h",
        "SceneSelectionPolicyApplier sceneSelectionPolicyApplierValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<EffectPolicyApplier> effectPolicyApplierValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<EffectRegistry> effectRegistryValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<EffectPresetCatalog> effectPresetCatalogValue");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "#include \"LegacySceneEffectControlTarget.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "#include \"LegacySceneEffectControlCatalog.h\"");
    assertSourceContains("src/SceneRuntime.cc",
        "#include \"RuntimeCommandTargets.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "#include \"EffectControlPolicy.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "#include \"EffectRegistry.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "#include \"EffectPresetCatalog.h\"");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "new SceneCommandsEffectControlOwner");
    assertSourceContains("src/LegacySceneEffectControlTarget.cc",
        "new SceneCommandsEffectControlOwner");
    assertSourceContains("src/CMakeLists.txt",
        "LegacySceneEffectControlTarget.cc");
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue->commandTarget()");
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue->effectControlOwner()");
    assertSourceDoesNotContain("src/Application.cc",
        "sceneRuntimeValue->effectControlTarget()");
    assertSourceContains("src/SceneChangeScheduler.cc",
        "sceneCommands.changeOne()");
    assertSourceContains("src/SceneChangeScheduler.cc",
        "sceneCommands.changeAll()");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "RuntimeCommandSink");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "RuntimeCommand::changeOne()");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "RuntimeCommand::changeAll()");
    assertSourceContains("src/InterfaceRuntime.h",
        "const SceneChangeStatusProvider* sceneChangeStatusProviderValue");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setSceneChangeStatusProvider(\n"
        "        sceneChangeSchedulerValue.get())");
    assertSourceContains("src/Application.cc", "class FrameGeneratorQuietObserver");
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
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "audioFrameMetrics");
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
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "audioMetrics.");
    assertSourceDoesNotExist("src/AutoChanger.h");
    assertSourceDoesNotExist("src/AutoChanger.cc");
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
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue,\n"
        "            frameGeneratorValue.quietMessageOption())");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue(randomSourceValue, countdownTimerFactoryValue,\n"
        "          logSinkValue)");
    assertSourceContains("src/Application.cc",
        "interfaceRuntimeValue->setAutoChangeControls(autoChangeControlsValue.get())");
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue->commandTarget()");
    assertSourceContains("src/Application.cc",
        "sceneChangeSchedulerValue.reset(new SceneChangeScheduler(sceneRuntimeValue->commandTarget()");
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
    assertSourceContains("src/SceneChangeScheduler.h", "MillisecondClock& clock");
    assertSourceContains("src/SceneChangeScheduler.h", "RandomSource& randomSource");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "gettime()");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "rand()");
    assertSourceContains("src/FrameGeneratorSceneBinding.h",
        "RandomSource& randomSourceValue");
    assertSourceContains("src/FrameGeneratorSceneBinding.cc",
        "silenceMessage.setRandomSource(randomSourceValue)");
    assertSourceDoesNotContain("src/FrameGeneratorSceneBinding.cc", "rand()");
    assertSourceContains("src/SilenceMessage.h", "RandomSource* randomSourceValue");
    assertSourceDoesNotContain("src/SilenceMessage.cc", "rand()");
    assertSourceDoesNotContain("src/DefaultMessagesProvider.cc", "rand()");
    assertSourceDoesNotContain("src/FileMessagesProvider.cc", "rand()");
    assertSourceContains("tests/unit/MessagesProviderTest.cc",
        "testDefaultMessagesUseInjectedRandomSource");
    assertSourceContains("src/Application.cc",
        "*autoChangeSettingsValue");
    assertSourceContains("src/SceneChangeScheduler.h",
        "const AutoChangeSettings& settings");
    assertSourceContains("src/SceneChangeScheduler.cc", "settings.quietMs()");
    assertSourceContains("src/SceneChangeScheduler.cc", "settings.waitMinMs()");
    assertSourceContains("src/SceneChangeScheduler.cc", "settings.waitRandomMs()");
    assertSourceContains("src/SceneChangeScheduler.cc",
        "settings.cumulativeFireLevel()");
    assertSourceContains("src/SceneChangeScheduler.cc", "settings.locked()");
    assertSourceContains("src/SceneChangeScheduler.cc", "settings.changeLittle()");
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
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionTime changeQuiet");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionOnOff lock");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h",
        "extern OptionOnOff change_little");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionTime changeQuiet");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionOnOff lock");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "OptionOnOff change_little");
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
    assertSourceContains("src/Application.h", "InputQueue inputQueueValue");
    assertSourceContains("src/Application.h", "CommandRegistry commandsValue");
    assertSourceContains("src/Application.h", "CommandDispatcher dispatcherValue");
    assertSourceContains("src/Application.h", "KeymapRegistry keymapsValue");
    assertSourceContains("src/Application.cc",
        "inputQueueValue.configure(startupConfigValue.input)");
    assertSourceContains("src/Application.cc",
        "processEvents(inputQueueValue)");
    assertSourceContains("src/Application.cc",
        "runCurrent(inputQueueValue, keymapsValue,");
    assertSourceContains("src/Application.cc",
        "commandsValue, dispatcherValue, commandContext)");
    assertSourceContains("src/Application.cc", "registerDefaultKeyActions(commandsValue)");
    assertSourceContains("src/Application.cc", "registerInterfaceKeyActions(commandsValue)");
    assertSourceContains("src/Application.cc",
        "keymapsValue.init(startupConfigValue.input, commandsValue)");
    assertSourceDoesNotContain("src/Application.cc", "Keymap::init");
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
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue->configureEffectPolicy(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "init_flashlight();\n"
        "    sceneRuntimeValue->configureEffectPolicy(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureTranslationOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureWaveOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configurePaletteOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "audioProcessingSelectorValue->configureStartup(startupConfigValue.scene)");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue.configure(startupConfigValue.display,\n"
        "        startupConfigValue.sceneTransition, startupConfigValue.messages)");
    assertSourceContains("src/SceneRuntime.h",
        "void applyStartupConfig(const SceneConfig& config)");
    assertSourceContains("src/SceneRuntime.cc",
        "commandsValue.applyStartupConfig(config)");
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue->applyStartupConfig(startupConfigValue.scene)");
    assertSourceDoesNotContain("src/Application.cc",
        "sceneCommands().applyStartupConfig(startupConfigValue.scene)");
    assertSourceContains("src/Application.cc",
        "applyDisplayPresentationStartupChoice(startupConfigValue.scene");
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
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "configureQuietMessages");
    assertSourceDoesNotContain("src/SceneChangeScheduler.h", "MessagesConfig");
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
    assertSourceContains("src/InputQueue.h", "class InputQueue");
    assertSourceContains("src/InputQueue.cc",
        "escapeKeyEnabled = config.escapeKeyEnabled");
    assertSourceDoesNotContain("src/keys.h", "configureKeys");
    assertSourceDoesNotContain("src/keys.h", "getkey()");
    assertSourceDoesNotContain("src/keys.h", "keys_x11");
    assertSourceDoesNotContain("src/keys.h", "extern int key_esc");
    assertSourceContains("src/keymap.h", "class CommandRegistry");
    assertSourceContains("src/keymap.h", "struct ActionInvocation");
    assertSourceContains("src/keymap.h", "class CommandContext");
    assertSourceContains("src/keymap.h", "class CommandDispatcher");
    assertSourceContains("src/keymap.h",
        "void init(const InputConfig& config, CommandRegistry& commands)");
    assertSourceDoesNotContain("src/keymap.h",
        "static void init(const InputConfig& config)");
    assertSourceDoesNotContain("src/keymap.h", "static Action* head");
    assertSourceDoesNotContain("src/keymap.cc", "Action* Action::head");
    assertSourceDoesNotContain("src/keymap.h", "a##ActionImpl");
    assertSourceContains("src/keymap.cc",
        "void registerDefaultKeyActions(CommandRegistry& registry)");
    assertSourceContains("src/keymap.cc",
        "int CommandDispatcher::dispatchKeymap");
    assertSourceContains("src/keymap.cc",
        "KeymapRegistry::actionsFor");
    assertSourceDoesNotContain("src/Configuration.h",
        "struct AppConfig {\n    int optionsSaveEnabled;\n    int escapeKeyEnabled");
    assertSourceDoesNotContain("src/keymap.h", "keymapFile[PATH_MAX]");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::configure");
    assertSourceDoesNotContain("src/keys.cc", "int x11_key");
    assertSourceDoesNotContain("src/keys.cc", "int key_esc");
    assertSourceDoesNotContain("src/keys.cc", "keys_x11");
    assertSourceDoesNotContain("src/keys.cc", "getkey()");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "keys_x11");
    assertSourceContains("src/DisplayDeviceX11.cc", "input.pushRawKey");
}

static void testSceneStartupUsesSceneConfig() {
    assertSourceContains("src/Configuration.h", "SceneConfig scene");
    assertSourceContains("src/Scene.cc", "SceneCommands::applyStartupConfig");
    assertSourceContains("src/SceneRuntime.h", "class SceneRuntime");
    assertSourceContains("src/SceneRuntime.h", "SceneRuntime(SceneGeometry& geometry");
    assertSourceContains("src/SceneRuntime.h",
        "SceneVisualCatalogFactory& visualCatalogFactory");
    assertSourceContains("src/SceneRuntime.h",
        "SceneVisualCatalogFactoryResult visualCatalogFactoryResultValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<SceneVisualCatalogs> visualCatalogsValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<SceneRuntimeControlBridge> controlBridgeValue");
    assertSourceContains("src/SceneRuntime.h", "SceneSelectionState selectionStateValue");
    assertSourceContains("src/SceneRuntime.h", "SceneCommands commandsValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<EffectRegistry> effectRegistryValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "std::unique_ptr<EffectPresetCatalog> effectPresetCatalogValue");
    assertSourceContains("src/SceneRuntime.h",
        "SceneSelectionRegistry sceneEffectRegistryValue");
    assertSourceContains("src/SceneRuntime.h",
        "SceneSelectionPresetCatalog sceneSelectionPresetCatalogValue");
    assertSourceContains("src/SceneRuntime.h",
        "SceneSelectionPolicyApplier sceneSelectionPolicyApplierValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "EffectPresetSceneCatalog scenePresetCatalogValue");
    assertSourceDoesNotContain("src/SceneRuntime.h", "SceneCommands& commands()");
    assertSourceDoesNotContain("src/SceneRuntime.h", "FrameRenderTarget");
    assertSourceDoesNotContain("src/SceneRuntime.cc", "#include \"FrameRenderTarget.h\"");
    assertSourceContains("src/SceneRuntime.h", "SceneSerializer serializerValue");
    assertSourceContains("src/SceneRuntime.cc",
        "SceneCommandDependencies(\n"
        "              *visualCatalogFactoryResultValue.visualCatalogs");
    assertSourceContains("src/SceneRuntime.cc",
        "sceneEffectRegistryValue, sceneSelectionPresetCatalogValue");
    assertSourceContains("src/SceneRuntime.cc",
        "sceneSelectionPolicyApplierValue(sceneEffectRegistryValue,\n"
        "          sceneSelectionPresetCatalogValue)");
    assertSourceContains("src/SceneRuntime.cc",
        "sceneSelectionPolicyApplierValue.configure(policy)");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "scenePresetCatalogValue(*effectPresetCatalogValue");
    assertSourceContains("src/SceneRuntime.cc",
        "visualCatalogFactory.create(selectionStateValue)");
    assertSourceContains("src/SceneRuntime.cc",
        "*visualCatalogFactoryResultValue.controlBridge");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "registerControls(*effectRegistryValue)");
    assertSourceContains("src/SceneRuntime.cc",
        "sceneEffectRegistryValue.registerSelections");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneSelectionRegistry : public SceneEffectRegistry");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "RegistrySceneEffectRegistry sceneEffectRegistryValue");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "legacyEffectControls");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "dynamic_cast<void*>");
    assertSourceContains("src/CMakeLists.txt", "SceneRuntime.cc");
    assertSourceContains("src/Scene.h", "class SceneCommandDependencies");
    assertSourceContains("src/Scene.h", "class SceneGeometry");
    assertSourceContains("src/Scene.h", "ScenePresetCatalog& presets");
    assertSourceDoesNotContain("src/Scene.h", "EffectPresetCatalog");
    assertSourceContains("src/SceneDependencies.h", "class SceneEffectRegistry");
    assertSourceContains("src/SceneDependencies.h", "class ScenePresetCatalog");
    assertSourceContains("src/CMakeLists.txt", "SceneRuntimeDependencies.cc");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.h",
        "class EffectPresetSceneCatalog");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneSelectionPresetCatalog : public ScenePresetCatalog");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneSelectionPolicyApplier");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class EffectPresetSceneCatalog");
    assertSourceContains("src/SceneGeometry.h", "class SceneGeometry");
    assertSourceContains("src/CMakeLists.txt", "SceneGeometry.cc");
    assertSourceDoesNotExist("src/CthughaBufferSceneGeometry.h");
    assertSourceDoesNotExist("src/CthughaBufferSceneGeometry.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt", "CthughaBufferSceneGeometry.cc");
    assertSourceDoesNotContain("src/SceneDependencies.h", "CthughaBuffer");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "#include \"CthughaBuffer.h\"");
    assertSourceDoesNotContain("src/SceneDependencies.h", "FrameRenderTarget");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "#include \"FrameRenderTarget.h\"");
    assertSourceContains("src/FrameGeometry.h",
        "class FrameGeometry : public SceneGeometry");
    assertSourceContains("src/FrameGeometry.h", "#include \"SceneGeometry.h\"");
    assertSourceContains("src/FrameGeometry.h", "virtual int width() const");
    assertSourceContains("src/FrameGeometry.h", "virtual int height() const");
    assertSourceContains("src/FrameGeneratorRuntime.h",
        "SceneGeometry& sceneGeometry()");
    assertSourceContains("src/Application.h",
        "FrameGeneratorRuntime frameGeneratorValue");
    assertSourceDoesNotContain("src/Application.h",
        "std::unique_ptr<SceneGeometry> sceneGeometryValue");
    assertSourceDoesNotContain("src/Application.h",
        "std::unique_ptr<SceneVisualSelections> sceneVisualSelectionsValue");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<SceneVisualCatalogFactory> sceneVisualCatalogFactoryValue");
    assertSourceDoesNotContain("src/Application.h", "LegacySceneVisualCatalogs.h");
    assertSourceDoesNotContain("src/Application.h", "SceneDependencies.h");
    assertSourceDoesNotContain("src/Application.h", "CthughaBufferSceneGeometry.h");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<SceneRuntime> sceneRuntimeValue");
    assertSourceContains("src/Application.h",
        "class SceneRuntime");
    assertSourceDoesNotContain("src/Application.cc",
        "#include \"CthughaBufferSceneGeometry.h\"");
    assertSourceDoesNotContain("src/Application.cc",
        "sceneGeometryValue");
    assertSourceContains("src/Application.cc",
        "createLegacySceneVisualCatalogFactory(\n"
        "            frameGeneratorValue.imageOption())");
    assertSourceDoesNotContain("src/Application.cc",
        "createLegacySceneVisualCatalogFactory(flame");
    assertSourceDoesNotContain("src/Application.cc",
        "sceneVisualSelectionsValue = createLegacySceneSelectionAdapters");
    assertSourceDoesNotContain("src/Application.cc",
        "#include \"LegacySceneSelectionAdapters.h\"");
    assertSourceDoesNotContain("src/Application.cc",
        "flameGeneral, wave,\n"
        "            waveScale, table, object");
    assertSourceContains("src/Application.cc",
        "sceneRuntimeValue.reset(new SceneRuntime(frameGeneratorValue.sceneGeometry(),\n"
        "        *sceneVisualCatalogFactoryValue, randomSourceValue))");
    assertSourceDoesNotContain("src/Application.cc",
        "new LegacySceneVisualCatalogFactory(*sceneVisualSelectionsValue)");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue.bindScene(sceneRuntimeValue->scene())");
    assertSourceContains("src/Application.cc",
        "*sceneVisualCatalogFactoryValue, randomSourceValue)");
    assertSourceDoesNotContain("src/Application.cc",
        "SceneCommandDependencies sceneDependencies");
    assertSourceDoesNotContain("src/Application.cc",
        "new SceneCommands(*sceneValue");
    assertSourceDoesNotContain("src/SceneRuntime.h",
        "CthughaBufferSceneGeometry geometryValue");
    assertSourceDoesNotContain("src/Application.h",
        "LegacySceneVisualCatalogs sceneVisualCatalogsValue");
    assertSourceContains("src/SceneDependencies.h", "class SceneVisualCatalogs");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class SceneOptionSelection");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class SceneVisualSelections");
    assertSourceContains("src/SceneVisualSelections.h", "class SceneOptionSelection");
    assertSourceContains("src/SceneVisualSelections.h", "class SceneVisualSelections");
    assertSourceContains("src/SceneVisualSelections.h",
        "const char* to, RandomSource& randomSource) = 0;\n"
        "    virtual int changeRandom(RandomSource& randomSource) = 0;");
    assertSourceContains("src/SceneVisualSelections.h",
        "virtual void activate(int index);");
    assertSourceContains("src/SceneVisualSelections.h",
        "virtual void toggleLock();");
    assertSourceContains("src/SceneVisualSelections.h",
        "virtual void toggleChoiceUse(int index);");
    assertSourceContains("src/SceneVisualSelections.h",
        "    virtual void setValue(int index) = 0;");
    assertSourceContains("src/CMakeLists.txt", "SceneVisualSelections.cc");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class SceneEffectControlSelection");
    assertSourceDoesNotExist("src/LegacySceneEffectControlSelection.h");
    assertSourceContains("src/LegacySceneEffectControlBindings.h",
        "class LegacySceneEffectControlBindings");
    assertSourceContains("src/LegacySceneEffectControlBindings.h",
        "SceneOptionSelection* selectionFor(EffectControl& option)");
    assertSourceContains("src/LegacySceneEffectControlBindings.h",
        "virtual void syncFromControls() = 0;");
    assertSourceContains("src/LegacySceneEffectControlBindings.h",
        "virtual void syncControlsFromSelections() = 0;");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "#include \"LegacySceneEffectControlBindings.h\"");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "#include \"LegacySceneEffectControlBindings.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"LegacySceneEffectControlBindings.h\"");
    assertSourceContains("src/SceneDependencies.h", "class SceneSelectionSynchronizer");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneRuntimeControlBridge : public SceneSelectionSynchronizer");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class SceneRuntimeControlBridge");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class SceneEffectControlCatalog");
    assertSourceContains("src/LegacySceneEffectControlCatalog.h",
        "class SceneEffectControlCatalog : public SceneRuntimeControlBridge");
    assertSourceContains("src/LegacySceneEffectControlCatalog.h",
        "virtual int isSceneOption(const EffectControl& option) const = 0;");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class EffectControl;");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "EffectControl& option");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneVisualCatalogFactoryResult");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "std::unique_ptr<SceneVisualCatalogs> visualCatalogs");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "std::unique_ptr<SceneRuntimeControlBridge> controlBridge");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneVisualCatalogFactory");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class SceneVisualCatalogFactoryResult");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class SceneVisualCatalogFactory");
    assertSourceContains("src/Scene.h", "class SceneSelectionState");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "SceneSelectionState& selectionState");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "selectionState.update(settings)");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "return selectionState.settings()");
    assertSourceContains("src/LegacySceneVisualCatalogs.h",
        "class LegacySceneVisualCatalogs : public SceneVisualCatalogs");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "public SceneEffectControlCatalog");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "class LegacySceneEffectControlCatalog : public SceneEffectControlCatalog");
    assertSourceContains("src/LegacySceneVisualCatalogs.h",
        "class LegacySceneVisualCatalogFactory : public SceneVisualCatalogFactory");
    assertSourceContains("src/LegacySceneVisualCatalogs.h",
        "std::unique_ptr<SceneVisualSelections> ownedSelections");
    assertSourceContains("src/LegacySceneVisualCatalogs.h",
        "std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory");
    assertSourceContains("src/LegacySceneVisualCatalogs.h",
        "ImageOption& images");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "class EffectControl;");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "EffectControl&");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "createLegacySceneVisualCatalogFactory");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "createLegacySceneSelectionAdapters(\n"
        "            flame, flameGeneral, wave, waveScale, table, object");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"display.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"flames.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"waves.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"Border.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"Flashlight.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "#include \"TranslationOptions.h\"");
    assertSourceContains("src/LegacySceneSelectionAdapters.h",
        "std::unique_ptr<SceneVisualSelections> createLegacySceneSelectionAdapters");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneSelectionAdapters : public SceneVisualSelections");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneFlameChoiceSelection flameValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneImageChoiceSelection imagesValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h",
        "class LegacySceneFlameSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h",
        "class LegacySceneImageSelection");
    assertSourceDoesNotContain("src/LegacySceneCatalogAdapters.cc",
        "class LegacySceneWaveObjectSource");
    assertSourceContains("src/LegacySceneCatalogAdapters.cc",
        "class LegacyScenePaletteRandomizer : public ScenePaletteRandomizer");
    assertSourceDoesNotContain("src/LegacySceneCatalogAdapters.h",
        "class SceneWaveObjectSource");
    assertSourceContains("src/LegacySceneCatalogAdapters.h",
        "class ScenePaletteRandomizer");
    assertSourceContains("src/LegacySceneCatalogAdapters.h",
        "virtual PaletteEntry* paletteEntry(int index) = 0");
    assertSourceDoesNotContain("src/LegacySceneCatalogAdapters.h",
        "createLegacySceneWaveObjectSource");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "std::unique_ptr<SceneWaveObjectSource> waveObjects");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "createLegacySceneWaveObjectSource");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "currentWaveObject()");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "currentSceneWaveObject(selections.object())");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "refreshOwnedPaletteEntry(selections, paletteRandomizer, index)");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "paletteSelection->replacePaletteEntry(index, *paletteEntry");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "paletteSelection->appendPaletteEntry(*paletteEntry");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "class LegacySceneWaveObjectSource");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h",
        "class LegacyScenePaletteRandomizer");
    assertSourceContains("src/CMakeLists.txt", "LegacySceneCatalogAdapters.cc");
    assertSourceContains("src/CMakeLists.txt", "LegacySceneEffectControlCatalog.cc");
    assertSourceContains("src/CMakeLists.txt", "LegacySceneSelectionAdapters.cc");
    assertSourceContains("src/CMakeLists.txt", "LegacySceneVisualCatalogs.cc");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h", "FlameOption&");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h", "GeneralFlameOption&");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h", "WaveOption&");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h", "TranslateOption&");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.h", "PaletteOption&");
    assertSourceDoesNotContain("src/SceneRuntime.h", "FlameOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "GeneralFlameOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "WaveOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "TranslateOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "PaletteOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "ImageOption");
    assertSourceDoesNotContain("src/SceneRuntime.h", "LegacySceneWaveObjectSource");
    assertSourceDoesNotContain("src/SceneDependencies.h", "LegacySceneWaveObjectSource");
    assertSourceDoesNotContain("src/SceneDependencies.h", "LegacyScenePaletteRandomizer");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class SceneWaveObjectSource");
    assertSourceDoesNotContain("src/SceneDependencies.h", "class ScenePaletteRandomizer");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "FlameOption");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "WaveOption");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "PaletteOption");
    assertSourceDoesNotContain("src/SceneDependencies.cc", "ImageOption");
    assertSourceContains("src/Scene.h", "SceneVisualCatalogs& visualCatalogs");
    assertSourceContains("src/Scene.cc",
        "dependencies.visualCatalogs.currentSettings(geometry)");
    assertSourceContains("src/Scene.cc",
        "dependencies.visualCatalogs.applyStartupConfig(config, randomSource)");
    assertSourceContains("src/LegacySceneEffectControlTarget.cc",
        "effectControls.isSceneOption(option)");
    assertSourceDoesNotContain("src/Scene.cc",
        "dependencies.effectControls.isSceneOption(option)");
    assertSourceContains("src/Scene.cc",
        "void SceneCommands::refreshFromOptionsAndMaybeCueImage");
    assertSourceContains("src/Scene.cc",
        "syncFromOptionsAndMaybeCueImage(dependencies.selectionSync.syncFromControls())");
    assertSourceContains("src/Scene.cc",
        "(forcedChanges & SceneImageChanged) != 0");
    assertSourceDoesNotContain("src/Scene.cc",
        "dependencies.effectControls.isImageOption");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "isImageOption");
    assertSourceDoesNotContain("src/LegacySceneEffectControlCatalog.cc",
        "isImageOption");
    assertSourceDoesNotContain("src/Scene.cc",
        "dependencies.visualCatalogs.isImageOption");
    assertSourceDoesNotContain("src/Scene.cc",
        "dependencies.visualCatalogs.isSceneOption");
    assertSourceDoesNotContain("src/VideoDirector.h", "VideoDirector& videoDirector()");
    assertSourceDoesNotContain("src/VideoDirector.cc", "VideoDirector& videoDirector()");
    assertSourceDoesNotContain("src/Scene.h", "sceneCommandsForLegacyCallbacks");
    assertSourceDoesNotContain("src/Scene.cc", "sceneCommandsForLegacyCallbacks");
    assertSourceDoesNotContain("src/Scene.cc", "applyStartupChoice(screen");
    assertSourceDoesNotContain("src/Scene.cc", "#include \"CthughaBuffer.h\"");
    assertSourceDoesNotContain("src/Scene.cc", "#include \"FrameRenderTarget.h\"");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc", "config.flame");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc", "config.wave");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc", "config.palette");
    assertSourceDoesNotContain("src/Scene.h", "FlameOption& flame");
    assertSourceDoesNotContain("src/Scene.h", "WaveOption& wave");
    assertSourceDoesNotContain("src/Scene.h", "PaletteOption& palette");
    assertSourceDoesNotContain("src/Scene.cc", "dependencies.flame");
    assertSourceDoesNotContain("src/Scene.cc", "dependencies.wave");
    assertSourceDoesNotContain("src/Scene.cc", "dependencies.palette");
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

static void testSceneSnapshotFlowsThroughFrameContext() {
    assertSourceContains("src/Scene.h", "class SceneSnapshot");
    assertSourceContains("src/Scene.h", "SceneSnapshot snapshot() const");
    assertSourceContains("src/Scene.cc",
        "return SceneSnapshot(settingsValue, versionValue)");
    assertSourceContains("src/FrameRenderContext.h", "class SceneSnapshot");
    assertSourceContains("src/FrameRenderContext.h",
        "const SceneSnapshot* sceneSnapshot");
    assertSourceContains("src/FrameRenderContext.cc", "sceneSnapshot(0)");
    assertSourceContains("src/Application.h",
        "const SceneSnapshot& sceneSnapshot");
    assertSourceContains("src/Application.cc",
        "context.sceneSnapshot = sceneSnapshot");
    assertSourceContains("src/Application.cc",
        "SceneSnapshot sceneSnapshot = sceneRuntimeValue->snapshot()");
    assertSourceContains("src/Application.cc",
        "runFrameGenerator(audioFrame, sceneSnapshot)");
    assertSourceContains("src/Application.cc",
        "acousticContextValue, &sceneSnapshot");
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
    assertSourceContains("src/SceneSerializer.h", "class SceneSerializer");
    assertSourceContains("src/SceneSerializer.cc",
        "config.scene.flame = persistedName(settings.flameName)");
    assertSourceContains("src/SceneSerializer.cc",
        "config.scene.image = persistedName(settings.imageName)");
    assertSourceContains("src/Application.cc",
        "runtimeConfigRegistryValue->addContributor(sceneRuntimeValue->serializer())");
    assertSourceDoesNotContain("src/Application.h",
        "std::unique_ptr<SceneSerializer> sceneSerializerValue");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "SceneCommands");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "sceneCommands");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "ImageOption");
    assertSourceContains("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessingState.text()");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessing.text()");
    assertSourceDoesNotContain("src/keymap.cc",
        "audioProcessing.text()");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "write_ini");
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc", "options_save");
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
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue,\n"
        "            frameGeneratorValue.quietMessageOption())");
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
        "class RuntimeEffectControlOwner");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "class RoutedRuntimeEffectControlTarget");
    assertSourceContains("src/RuntimeCommandTargets.h",
        "RuntimeEffectControlOwner& effectControlOwner");
    assertSourceDoesNotContain("src/RuntimeCommandTargets.h",
        "SceneEffectControlTarget");
    assertSourceDoesNotContain("src/RuntimeCommandTargets.cc",
        "#include \"LegacySceneEffectControlTarget.h\"");
    assertSourceDoesNotContain("src/RuntimeCommandTargets.h",
        "SceneCommands& sceneCommands");
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
        "CommandContext commandContext(*interfaceRuntimeValue,");
    assertSourceContains("src/Application.cc",
        "runtimeCommandRouterValue.get())");
    assertSourceDoesNotContain("src/Application.cc",
        "interfaceRuntimeValue->setCommandRouter");
    assertSourceDoesNotContain("src/InterfaceRuntime.h", "commandRouterValue");
    assertSourceContains("src/keymap.h",
        "RuntimeCommandTargetRouter* commandRouterValue");
    assertSourceContains("src/keymap.cc",
        "commandRouterValue->toggleEffectChoiceUse");
    assertSourceContains("src/keymap.cc",
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
        "context.toggleEffectChoiceUse()");
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
        "#include \"SceneChangeScheduler.h\"");
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
    assertSourceContains("src/DisplayDevice.h", "ImageOption& images");
    assertSourceContains("src/xcthugha.h", "ImageOption& images");
    assertSourceDoesNotContain("src/DisplayDevice.h", "SceneCommands");
    assertSourceDoesNotContain("src/xcthugha.h", "SceneCommands");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "SceneCommands& sceneCommands");
    assertSourceDoesNotContain("src/Application.h",
        "SceneCommands& sceneCommands");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue.imageOption(), *runtimeChangeMediatorValue");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "currentNameOrEmpty(images)");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "add_menu(\"Image\", &images");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc",
        "d->opt->setValue");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc",
        "sceneCommands.imageOption()");
    assertSourceDoesNotContain("src/Scene.h", "imageOption()");
    assertSourceDoesNotContain("src/Scene.cc", "SceneCommands::imageOption");
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
    assertSourceContains("src/Application.h",
        "std::unique_ptr<ErrorMessages> errorMessagesValue");
    assertSourceContains("src/Application.cc",
        "registerDefaultInterfaces(*interfaceRuntimeValue,\n"
        "        frameGeneratorValue.imageOption(),\n"
        "        frameGeneratorValue.quietMessageOption())");
    assertSourceDoesNotContain("src/keymap.h", "setInterfaceRuntime");
    assertSourceDoesNotContain("src/keymap.h", "interfaceRuntime");
    assertSourceDoesNotContain("src/keymap.cc", "keymapInterfaceRuntime");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::setInterfaceRuntime");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::interfaceRuntime");
    assertSourceDoesNotContain("src/Application.cc",
        "Keymap::setInterfaceRuntime");
    assertSourceDoesNotContain("src/InterfaceRuntime.h",
        "RuntimeCommandSink* runtimeCommandSinkValue");
    assertSourceContains("src/keymap.h",
        "RuntimeCommandSink* runtimeCommandSinkValue");
    assertSourceContains("src/InterfaceRuntime.h",
        "std::vector<Interface*> ownedInterfacesValue");
    assertSourceContains("src/InterfaceRuntime.cc",
        "void InterfaceRuntime::registerOwnedInterface");
    assertSourceContains("src/Interface.cc",
        "runtime.registerOwnedInterface(new InterfaceMain())");
    assertSourceContains("src/Interface.cc",
        "runtime.registerOwnedInterface(new Interface(\"Mixer\", \"Mixer\", NULL))");
    assertSourceContains("src/Interface.cc",
        "runtime.registerOwnedInterface(new InterfaceEffectControl(images))");
    assertSourceContains("src/Interface.cc",
        "runtime.registerOwnedInterface(new InterfaceOptions(quietMessageOption))");
    assertSourceContains("src/AudioSystem.cc",
        "runtime.registerOwnedInterface(");
    assertSourceContains("src/InterfaceList.cc",
        "runtime.registerOwnedInterface(");
    assertSourceDoesNotContain("src/Interface.cc",
        "} interfaceMain");
    assertSourceDoesNotContain("src/Interface.cc",
        "static Interface interfaceMixer");
    assertSourceDoesNotContain("src/Interface.cc",
        "} interfaceEffectControl");
    assertSourceDoesNotContain("src/Interface.cc",
        "Interface interfaceOption");
    assertSourceDoesNotContain("src/Interface.cc",
        "elementsOption[]");
    assertSourceDoesNotContain("src/AudioSystem.cc",
        "Interface interfacePlayList");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "InterfaceList interfaceList");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "int InterfaceList::size");
    assertSourceContains("src/InterfaceRuntime.h",
        "char extraKeymapValue[512]");
    assertSourceContains("src/Interface.cc",
        "ACTION(setExtraKeymap) { context.runtime().setExtraKeymap(invocation.param); }");
    assertSourceDoesNotContain("src/Interface.cc",
        "interfaceMain.extraKeymap");
    assertSourceDoesNotContain("src/Interface.cc",
        "friend class setExtraKeymapAction");
    assertSourceContains("src/InterfaceRuntime.h",
        "double helpScrollPositionValue");
    assertSourceContains("src/InterfaceRuntime.h",
        "int helpScrollingValue");
    assertSourceContains("src/InterfaceHelp.cc",
        "ACTION(toggleScrolling) { context.runtime().toggleHelpScrolling(); }");
    assertSourceContains("src/InterfaceHelp.cc",
        "context.runtime().scrollHelpBy(-1.0, InterfaceHelp::lineCount())");
    assertSourceContains("src/InterfaceHelp.cc",
        "runtime.registerOwnedInterface(new InterfaceHelp())");
    assertSourceDoesNotContain("src/InterfaceHelp.cc",
        "} interfaceHelp");
    assertSourceDoesNotContain("src/InterfaceHelp.cc",
        "interfaceHelp.scrolling");
    assertSourceDoesNotContain("src/InterfaceHelp.cc",
        "interfaceHelp.pos");
    assertSourceDoesNotContain("src/InterfaceHelp.cc",
        "friend class toggleScrollingAction");
    assertSourceContains("src/InterfaceRuntime.h",
        "double creditsPositionValue");
    assertSourceContains("src/InterfaceRuntime.h",
        "int creditsFirstTimeValue");
    assertSourceContains("src/InterfaceCredits.cc",
        "runtime.updateCreditsPosition(currentTime");
    assertSourceContains("src/InterfaceCredits.cc",
        "runtime.registerOwnedInterface(new InterfaceCredits())");
    assertSourceDoesNotContain("src/InterfaceCredits.cc",
        "} interfaceCredits");
    assertSourceDoesNotContain("src/InterfaceCredits.cc",
        "int firstTime");
    assertSourceDoesNotContain("src/InterfaceCredits.cc",
        "    double pos;");
    assertSourceContains("src/Application.cc",
        "runtimeChangeMediatorValue.get(), runtimeCommandRouterValue.get())");
    assertSourceDoesNotContain("src/Application.cc",
        "interfaceRuntimeValue->setRuntimeCommandSink");
    assertSourceDoesNotContain("src/keymap.h", "setRuntimeCommandSink");
    assertSourceDoesNotContain("src/keymap.cc", "keymapRuntimeCommandSink");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::runtimeCommandSink");
    assertSourceDoesNotContain("src/keymap.h", "static Keymap* current");
    assertSourceDoesNotContain("src/keymap.h", "static void set");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::current");
    assertSourceDoesNotContain("src/keymap.cc", "void Keymap::set");
    assertSourceDoesNotContain("src/keymap.h", "static Keymap* first");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap* Keymap::first");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::find");
    assertSourceDoesNotContain("src/keymap.h", "static Keymap* find");
    assertSourceDoesNotContain("src/keymap.h",
        "static int action(const char* name");
    assertSourceDoesNotContain("src/keymap.cc",
        "int Keymap::action(const char*");
    assertSourceDoesNotContain("src/keymap.cc", "int Keymap::action(int");
    assertSourceDoesNotContain("src/keymap.h", "int action(int key");
    assertSourceContains("src/keymap.h", "char* actionName");
    assertSourceContains("src/keymap.h",
        "std::vector<ActionInvocation>& invocations");
    assertSourceContains("src/keymap.cc", "commands.find(a->actionName)");
    assertSourceContains("src/keymap.cc",
        "invocations.push_back(ActionInvocation");
    assertSourceContains("src/keymap.cc",
        "CommandDispatcher::dispatchKeymap");
    assertSourceDoesNotContain("src/keymap.h", "static Action* head");
    assertSourceDoesNotContain("src/keymap.cc", "Action* Action::head");
    assertSourceDoesNotContain("src/keymap.h", "a##ActionImpl");
    assertSourceContains("src/keymap.cc", "findLongestPrefix(line)");
    assertSourceContains("src/Interface.cc",
        "void registerInterfaceKeyActions(CommandRegistry& registry)");
    assertSourceContains("src/InterfaceList.cc",
        "void registerListKeyActions(CommandRegistry& registry)");
    assertSourceContains("src/InterfaceHelp.cc",
        "void registerHelpKeyActions(CommandRegistry& registry)");
    assertSourceContains("src/keymap.cc", "Keymap* keymap = find(\"default\", 1)");
    assertSourceContains("src/keymap.cc", "keymap = find(lineP, 1)");
    assertSourceDoesNotContain("src/keymap.cc", "static Keymap");
    assertSourceDoesNotContain("src/Interface.h", "#include \"keymap.h\"");
    assertSourceDoesNotContain("src/Interface.h", "static Keymap keymap");
    assertSourceDoesNotContain("src/Interface.h",
        "static Keymap effectControlKeymap");
    assertSourceDoesNotContain("src/Interface.cc",
        "Keymap InterfaceElementOption::keymap");
    assertSourceDoesNotContain("src/Interface.cc",
        "Keymap InterfaceElementEffectControl::effectControlKeymap");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "Keymap InterfaceList::listOptionKeymap");
    assertSourceContains("src/Interface.cc",
        "runtime.runOptionKey(*opt, *this, keymaps, commands, dispatcher,");
    assertSourceContains("src/Interface.cc",
        "dispatcher.dispatchKeymap(keymaps, commands");
    assertSourceContains("src/CthughaDisplayX11.cc",
        "CurrentInterfaceOverlayProducer(InterfaceRuntime& runtime_)");
    assertSourceContains("src/CthughaDisplayX11.cc",
        "currentInterface->display(runtime)");
    assertSourceContains("src/CthughaDisplayX11.cc",
        "ErrorMessagesOverlayProducer errorProducer(errorMessages, runtime)");
    assertSourceDoesNotContain("src/Interface.h", "extern ErrorMessages");
    assertSourceDoesNotContain("src/Interface.cc", "ErrorMessages errors");
    assertSourceDoesNotContain("src/CthughaDisplayX11.cc",
        "ErrorMessagesOverlayProducer errorProducer(errors");

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
        "sceneChangeStatusProviderValue =");
    assertSourceDoesNotContain("src/Interface.cc",
        "autoChangeControlsValue =");
    assertSourceDoesNotContain("src/Interface.cc",
        "audioProcessingSelectorValue =");
    assertSourceDoesNotContain("src/CthughaDisplayX11.cc",
        "Interface::current");
}

static void testTranslationGenerationUsesApplicationRandomSource() {
    assertSourceContains("src/Application.cc",
        "initializeVisualCatalogs(frameGeneratorValue.geometry(),\n"
        "            startupConfigValue.paths, randomSourceValue, logSinkValue)");
    assertSourceContains("src/Application.cc", "init_translate(geometry, randomSource)");
    assertSourceContains("src/TranslationOptions.h",
        "int init_translate(const SceneGeometry& geometry, RandomSource& randomSource)");
    assertSourceContains("src/TranslationOptions.cc",
        "defaultTranslationCatalog().generateAll(geometry.width(), geometry.height(),\n"
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
    assertSourceContains("src/FrameGeneratorSceneBinding.cc",
        "randomPaletteTransitionStrategy(randomSourceValue)");
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
    assertSourceContains("src/PaletteEntry.h",
        "static void addRandom(RandomSource& randomSource)");
    assertSourceContains("src/PaletteEntry.h",
        "static void randomizeLast(RandomSource& randomSource)");
    assertSourceContains("src/PaletteEntry.h",
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
    assertSourceDoesNotContain("src/PaletteEntry.h", "static void Random()");
    assertSourceContains("src/LegacySceneCatalogAdapters.cc",
        "PaletteEntry::randomizeLast(randomSource)");
    assertSourceContains("src/LegacySceneCatalogAdapters.cc",
        "PaletteEntry::addRandom(randomSource)");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "PaletteEntry::randomizeLast(randomSource)");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "PaletteEntry::addRandom(randomSource)");
    assertSourceContains("src/Scene.cc",
        "dependencies.visualCatalogs.randomPalette(randomSource)");
    assertSourceContains("src/Scene.cc",
        "dependencies.visualCatalogs.addRandomPalette(randomSource)");
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
        "lookupTables, randomSource, log, fireBudget)");
    assertSourceContains("src/FrameFilters.h",
        "void setRandomSource(RandomSource& randomSource)");
    assertSourceContains("src/FrameFilters.cc",
        "wave->execute(frame.buffer(), frame.context(), config,\n"
        "            needsConfiguration, state, lookupTables, *randomSourceValue,\n"
        "            frame.log())");
    assertSourceContains("src/FrameGeneratorSceneBinding.cc",
        "waveFilter->setRandomSource(randomSourceValue)");
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
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue(randomSourceValue, countdownTimerFactoryValue,\n"
        "          logSinkValue)");
    assertSourceDoesNotContain("src/FrameGeneratorSceneBinding.cc",
        "CthughaBuffer::buffer");
    assertSourceContains("src/Image.h",
        "int bufferHeight, RandomSource& randomSource) const");
    assertSourceContains("src/Image.cc",
        "randomSource.uniformInt(imageSize - bufferSize + 1)");
    assertSourceContains("src/Image.cc",
        "randomSource.uniformInt(bufferSize - imageSize + 1)");
    assertSourceDoesNotContain("src/Image.cc", "Random(");
    assertSourceContains("src/FrameGeneratorSceneBinding.cc",
        "geometryValue.width(), geometryValue.height(), randomSourceValue)");
    assertSourceContains("tests/unit/ImagePlacementTest.cc",
        "testSmallImagePlacementUsesInjectedRandomSource");
}

static void testGeneralFlameUsesInjectedRandomSource() {
    assertSourceContains("src/Scene.h", "RandomSource& randomSource");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "palette, border, flashlight, images)");
    assertSourceDoesNotContain("src/Application.cc",
        "flashlight, frameGeneratorValue.imageOption());");
    assertSourceContains("src/Application.cc",
        "createLegacySceneVisualCatalogFactory(\n"
        "            frameGeneratorValue.imageOption())");
    assertSourceContains("src/Application.cc",
        "*sceneVisualCatalogFactoryValue, randomSourceValue)");
    assertSourceContains("src/SceneRuntime.cc",
        "RandomSource& randomSource)");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "case SceneSelectionGeneralFlame");
    assertSourceDoesNotContain("src/Scene.h", "changeGeneralFlame");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "selections.generalFlame().changeRandom(randomSource)");
    assertSourceContains("src/SceneGeneralFlameSelectionValue.cc",
        "setValue(randomSource.uniformInt(generalFlameStates))");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "generalFlameOption.changeRandom(randomSource, 0)");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "selections.generalFlame(), config.generalFlame");
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
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "selection->change(to, randomSource)");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "option.change(to, randomSource, doSave)");
    assertSourceContains("src/LegacySceneEffectControlTarget.cc",
        "effectRegistry.saveAll()");
    assertSourceDoesNotContain("src/Scene.cc",
        "dependencies.effectRegistry.saveAll()");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "EffectControl::save()");
    assertSourceContains("src/Scene.cc",
        "dependencies.effectRegistry.changeAll(randomSource)");
    assertSourceContains("src/Scene.cc",
        "dependencies.effectRegistry.changeOne(randomSource)");
    assertSourceContains("src/Scene.cc",
        "dependencies.selectionSync.syncFromControls()");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "return imageChangeFrom(previousImageValue)");
    assertSourceContains("tests/unit/SceneCommandsSyncTest.cc",
        "testChangeOneUsesSyncReturnedImageChangeForCue");
    assertSourceContains("src/EffectRegistry.cc",
        "void EffectRegistry::changeAll(RandomSource& randomSource)");
    assertSourceContains("src/EffectRegistry.cc",
        "EffectControl* EffectRegistry::changeOne(RandomSource& randomSource)");
    assertSourceContains("src/EffectRegistry.cc",
        "saveAll();\n    selected->changeRandom(randomSource, 0)");
    assertSourceDoesNotContain("src/EffectRegistry.cc",
        "selected->changeRandom(randomSource, 1)");
    assertSourceContains("tests/unit/EffectControlRandomTest.cc",
        "testEffectRegistryChangeOneSavesOnlyExplicitControls");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionRegistry::changeAll(RandomSource& randomSource)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "entries[i].selection->changeRandom(randomSource)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionRegistry::changeOne(RandomSource& randomSource)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "selection->changeRandom(randomSource)");
    assertSourceContains("tests/unit/SceneCommandsSyncTest.cc",
        "testSceneSelectionRegistryOwnsSelectionHistoryAndRandomChanges");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionPresetCatalog::save(int slot)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "slots[slot].saveCurrentValues(registry)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionPresetCatalog::restore(int slot)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "slots[slot].restoreValues(registry)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionPresetCatalog::setValue(");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "void SceneSelectionPolicyApplier::configure(const EffectPolicy& policy)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "applyAllowedChoicePolicy(selection, allowedChoices)");
    assertSourceContains("src/SceneRuntimeDependencies.cc",
        "applyPresetPolicy(selection, presetPolicies, presets)");
    assertSourceContains("tests/unit/SceneCommandsSyncTest.cc",
        "testSceneSelectionPresetCatalogOwnsSelectionSnapshots");
    assertSourceContains("tests/unit/SceneCommandsSyncTest.cc",
        "testSceneSelectionPolicyApplierUsesSceneSelections");
    assertSourceDoesNotContain("tests/unit/SceneCommandsSyncTest.cc",
        "testEffectPresetSceneCatalogFallsBackToLegacyAndOverridesWithSceneSlot");
    assertSourceDoesNotContain("src/Scene.cc",
        "#include \"EffectPresetCatalog.h\"");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.cc",
        "legacyPresets");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.cc",
        "EffectPresetSceneCatalog");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.h",
        "EffectPresetCatalog");
    assertSourceContains("tests/unit/SceneCommandsSyncTest.cc",
        "class RecordingPresetCatalog : public ScenePresetCatalog");
    assertSourceDoesNotContain("tests/unit/SceneCommandsSyncTest.cc",
        "#include \"EffectPresetCatalog.h\"");
    assertSourceContains("src/EffectControlPolicy.h",
        "class EffectPolicyApplier");
    assertSourceContains("src/EffectControlPolicy.cc",
        "void EffectPolicyApplier::applyAll()");
    assertSourceDoesNotContain("src/EffectControlPolicy.cc",
        "activeEffectPolicyApplier");
    assertSourceDoesNotContain("src/EffectControlPolicy.h",
        "configureEffectPolicy(");
    assertSourceDoesNotContain("src/EffectControlPolicy.h",
        "effectControlPolicyObserve(");
    assertSourceDoesNotContain("src/EffectControl.cc",
        "#include \"EffectControlPolicy.h\"");
    assertSourceDoesNotContain("src/EffectControl.cc",
        "effectControlPolicyObserve(");
    assertSourceDoesNotContain("src/EffectPresetCatalog.cc",
        "EffectControl::firstRegistered()");
    assertSourceDoesNotContain("src/EffectControlPolicy.cc",
        "EffectControl::firstRegistered()");
    assertSourceContains("src/SceneRuntimeDependencies.h",
        "class SceneSelectionRegistry : public SceneEffectRegistry");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.h",
        "class RegistrySceneEffectRegistry");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class RegistrySceneEffectRegistry");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "class LegacySceneEffectRegistry");
    assertSourceDoesNotContain("src/SceneDependencies.cc",
        "EffectControl::changeAll(randomSource)");
    assertSourceDoesNotContain("src/SceneDependencies.cc",
        "EffectControl::changeOne(randomSource)");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.cc",
        "registry.changeAll(randomSource)");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.cc",
        "registry.changeOne(randomSource)");
    assertSourceDoesNotContain("src/SceneDependencies.h",
        "EffectControl* changeOne");
    assertSourceDoesNotContain("src/SceneRuntimeDependencies.cc",
        "EffectControl* RegistrySceneEffectRegistry::changeOne");
    assertSourceContains("src/SceneRuntime.cc",
        "createEffectControlOwner(");
    assertSourceContains("src/LegacySceneEffectControlTarget.cc",
        "SceneEffectControlCatalog::createEffectControlOwner");
    assertSourceDoesNotContain("src/SceneRuntime.cc",
        "legacyEffectControls");
    assertSourceDoesNotContain("src/LegacySceneEffectControlCatalog.cc",
        "registerSelection(registry");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "legacySceneEffectControlBindings(selections_)");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "bindings->syncFromControls()");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "bindings->syncControlsFromSelections()");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "syncLegacyControlsFromSelections(selections)");
    assertSourceContains("src/LegacySceneVisualCatalogs.cc",
        "syncLegacyControlsAndReturn(selections, result)");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "registerSelection(registry");
    assertSourceDoesNotContain("src/LegacySceneVisualCatalogs.cc",
        "registerControl(registry)");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "registry.registerControl(option)");
    assertSourceContains("src/SceneChoiceSelection.h",
        "class SceneChoiceCatalog");
    assertSourceContains("src/SceneChoiceSelection.h", "class SceneChoice");
    assertSourceContains("src/SceneChoiceSelection.h", "class SceneChoiceLock");
    assertSourceContains("src/SceneChoiceSelection.h",
        "class SceneChoiceSelection : public virtual SceneOptionSelection");
    assertSourceContains("src/SceneChoiceSelection.h",
        "virtual void toggleLock();");
    assertSourceContains("src/SceneChoiceSelection.h",
        "virtual void toggleChoiceUse(int index);");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "selection->toggleLock()");
    assertSourceContains("src/LegacySceneEffectControlCatalog.cc",
        "selection->toggleChoiceUse(index)");
    assertSourceDoesNotContain("src/LegacySceneEffectControlCatalog.cc",
        "option.lock.change");
    assertSourceDoesNotContain("src/LegacySceneEffectControlCatalog.cc",
        "option[index]->setUse");
    assertSourceDoesNotContain("src/SceneChoiceSelection.h",
        "#include \"EffectControl.h\"");
    assertSourceDoesNotContain("src/SceneChoiceSelection.h", "EffectControl&");
    assertSourceDoesNotContain("src/SceneChoiceSelection.h", "EffectChoice");
    assertSourceDoesNotContain("src/SceneChoiceSelection.h", "OptionOnOff");
    assertSourceDoesNotContain("src/SceneChoiceSelection.cc",
        "#include \"EffectControl.h\"");
    assertSourceDoesNotContain("src/SceneChoiceSelection.cc", "EffectControl");
    assertSourceDoesNotContain("src/SceneChoiceSelection.cc", "EffectChoice");
    assertSourceDoesNotContain("src/SceneChoiceSelection.cc", "OptionOnOff");
    assertSourceContains("src/CMakeLists.txt", "SceneChoiceListCatalog.cc");
    assertSourceContains("src/CMakeLists.txt", "SceneChoiceSelection.cc");
    assertSourceContains("src/CMakeLists.txt", "LegacySceneChoiceLock.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt", "SceneEffectChoiceCatalog.cc");
    assertSourceContains("src/CMakeLists.txt",
        "SceneGeneralFlameSelectionValue.cc");
    assertSourceContains("src/CMakeLists.txt", "SceneTypedVisualCatalogs.cc");
    assertSourceContains("tests/CMakeLists.txt",
        "scene_choice_list_catalog_test");
    assertSourceContains("tests/CMakeLists.txt",
        "legacy_scene_choice_lock_test");
    assertSourceDoesNotContain("tests/CMakeLists.txt",
        "scene_effect_choice_catalog_test");
    assertSourceContains("tests/CMakeLists.txt",
        "scene_typed_visual_catalogs_test");
    assertSourceContains("tests/CMakeLists.txt",
        "scene_general_flame_selection_value_test");
    assertSourceDoesNotExist("src/SceneEffectChoiceCatalog.h");
    assertSourceDoesNotExist("src/SceneEffectChoiceCatalog.cc");
    assertSourceDoesNotExist("tests/unit/SceneEffectChoiceCatalogTest.cc");
    assertSourceContains("src/LegacySceneChoiceLock.h",
        "class LegacySceneChoiceLock : public SceneChoiceLock");
    assertSourceContains("src/SceneChoiceListCatalog.h",
        "class SceneChoiceListCatalog : public SceneChoiceCatalog");
    assertSourceDoesNotContain("src/SceneChoiceListCatalog.h",
        "EffectControl");
    assertSourceDoesNotContain("src/SceneChoiceListCatalog.cc",
        "EffectControl");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "#include \"SceneChoiceListCatalog.h\"");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "#include \"LegacySceneChoiceLock.h\"");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class EffectControlSceneChoice");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class EffectControlSceneChoiceCatalog");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "#include \"SceneGeneralFlameSelectionValue.h\"");
    assertSourceContains("src/SceneGeneralFlameSelectionValue.h",
        "class SceneGeneralFlameSelectionValue : public SceneGeneralFlameSelection");
    assertSourceDoesNotContain("src/SceneGeneralFlameSelectionValue.h",
        "EffectControl");
    assertSourceDoesNotContain("src/SceneGeneralFlameSelectionValue.cc",
        "EffectControl");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneFlameChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneFlameChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneFlameChoiceSelection : public SceneChoiceSelection");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveChoiceSelection : public SceneChoiceSelection");
    assertSourceContains("src/SceneVisualSelections.h",
        "class SceneWaveObjectSelection : public virtual SceneOptionSelection");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveObjectChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveObjectChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneWaveObjectChoiceSelection : public SceneChoiceSelection");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneTranslationChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneTranslationChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneTranslationChoiceSelection : public SceneChoiceSelection");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class ScenePaletteChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class ScenePaletteChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class ScenePaletteChoiceSelection : public SceneChoiceSelection");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "int appendPaletteEntry(const PaletteEntry& palette, int inUse)");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "int replacePaletteEntry(int index, const PaletteEntry& palette, int inUse)");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneImageChoice : public SceneChoice");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneImageChoiceCatalog : public SceneChoiceCatalog");
    assertSourceContains("src/SceneTypedVisualCatalogs.h",
        "class SceneImageChoiceSelection : public SceneChoiceSelection");
    assertSourceDoesNotContain("src/SceneTypedVisualCatalogs.h",
        "EffectControl");
    assertSourceDoesNotContain("src/SceneTypedVisualCatalogs.cc",
        "EffectControl");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneControlBackedSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneEffectChoiceSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneFlameSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneWaveSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneTranslationSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacyScenePaletteSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneImageSelection");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneFlameChoiceSelection flameValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneGeneralFlameSelectionValue generalFlameValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneWaveChoiceSelection waveValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneChoiceSelection waveScaleValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneChoiceSelection tableValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneWaveObjectChoiceSelection objectValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneTranslationChoiceSelection translationValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "ScenePaletteChoiceSelection paletteValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneChoiceSelection borderValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneChoiceSelection flashlightValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "SceneImageChoiceSelection imagesValue;");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "flameValue(createSceneFlameChoiceCatalog(flame_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "waveValue(createSceneWaveChoiceCatalog(wave_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "waveScaleValue(createWaveScaleChoiceCatalog(waveScale_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "tableValue(createTableChoiceCatalog(table_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "objectValue(createSceneWaveObjectChoiceCatalog(object_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "translationValue(createSceneTranslationChoiceCatalog(translation_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "paletteValue(createScenePaletteChoiceCatalog(palette_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "borderValue(createBorderChoiceCatalog(border_)");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "flashlightValue(createFlashlightChoiceCatalog(flashlight_)");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "createOwnedSceneChoiceCatalog");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "imagesValue(createSceneImageChoiceCatalog(images_)");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "flameValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "waveValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "borderValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "flashlightValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "waveScaleValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "tableValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "objectValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "imagesValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "translationValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "paletteValue(new SceneEffectChoiceCatalog");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection waveScaleValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection tableValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection objectValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection borderValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection flashlightValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "LegacySceneControlBackedSelection generalFlameValue;");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "public SceneEffectControlSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneOptionSelection");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "class LegacySceneChoiceCatalog");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "int selectedValue");
    assertSourceContains("src/SceneChoiceSelection.h",
        "std::unique_ptr<SceneChoiceCatalog> catalog");
    assertSourceContains("src/SceneChoiceSelection.h",
        "std::vector<SceneChoice*> choices");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "void SceneChoiceSelection::refreshCatalog() const");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "choices.push_back(catalog->choiceAt(i))");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "return int(choices.size())");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "SceneChoice* choice = choiceAt(i)");
    assertSourceContains("src/SceneChoiceSelection.cc",
        "void SceneChoiceSelection::selectionChanged() { }");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "void LegacySceneControlBackedSelection::selectionChanged()");
    assertSourceContains("src/LegacySceneSelectionAdapters.cc",
        "generalFlameControl.setValue(generalFlameValue.currentValue())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "void SceneChoiceSelection::mirrorToControl()");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "EffectControl& SceneChoiceSelection::control()");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "choices.push_back(const_cast<EffectControl&>(option)[i])");
    assertSourceContains("src/SceneGeneralFlameSelectionValue.cc",
        "return selectedValue");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<SceneFlameChoice*>(currentChoice())");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<SceneWaveChoice*>(currentChoice())");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<SceneWaveObjectChoice*>(currentChoice())");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<SceneTranslationChoice*>(currentChoice())");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<ScenePaletteChoice*>(currentChoice())");
    assertSourceContains("src/SceneTypedVisualCatalogs.cc",
        "dynamic_cast<SceneImageChoice*>(currentChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "dynamic_cast<FlameEntry*>(currentEffectChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "dynamic_cast<WaveEntry*>(currentEffectChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "dynamic_cast<TranslateEntry*>(currentEffectChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "dynamic_cast<PaletteEntry*>(currentEffectChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "dynamic_cast<ImageEntry*>(currentEffectChoice())");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc",
        "translationOption.translationTable(currentValue())");
    assertSourceContains("src/LegacySceneSelectionAdapters.h",
        "EffectControl& flame, EffectControl& generalFlame, EffectControl& wave");
    assertSourceContains("src/LegacySceneSelectionAdapters.h",
        "EffectControl& translation, EffectControl& palette, EffectControl& border");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "FlameOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "GeneralFlameOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "WaveOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "TranslateOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "PaletteOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.h", "ImageOption");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "FlameOption&");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "GeneralFlameOption&");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "WaveOption&");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "TranslateOption&");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "PaletteOption&");
    assertSourceDoesNotContain("src/LegacySceneSelectionAdapters.cc", "ImageOption&");
    assertSourceDoesNotContain("src/Application.cc",
        "effectRegistryValue.registerControl(flame)");
    assertSourceContains("tests/unit/EffectControlRandomTest.cc",
        "testStaticRandomChangesUseInjectedRandomSource");
    assertSourceContains("tests/unit/EffectControlRandomTest.cc",
        "testEffectRegistryRandomChangesUseExplicitControls");
    assertSourceContains("tests/CMakeLists.txt", "effect_control_random_test");
}

static void testQotdTimingUsesApplicationCountdownTimers() {
    assertSourceContains("src/ProcessServices.h", "class CountdownTimer");
    assertSourceContains("src/ProcessServices.h", "class CountdownTimerFactory");
    assertSourceContains("src/ProcessServices.h", "class SystemCountdownTimerFactory");
    assertSourceContains("src/Application.h",
        "SystemCountdownTimerFactory countdownTimerFactoryValue");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue(randomSourceValue, countdownTimerFactoryValue,\n"
        "          logSinkValue)");
    assertSourceContains("src/FrameGeneratorSceneBinding.h",
        "RandomSource& randomSource, CountdownTimerFactory& timerFactory,\n"
        "        LogSink& log)");
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
    assertSourceDoesNotContain("src/SceneChangeScheduler.cc",
        "DEFAULT_CHANGE_QUIET_MS");
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
    testSceneSnapshotFlowsThroughFrameContext();
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
