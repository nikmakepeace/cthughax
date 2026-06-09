/** @file
 * Startup-time runtime composition helpers.
 */

#include "config.h"
#include "RuntimeFactory.h"
#include "ProcessServices.h"

#include <memory>
#include <unistd.h>

Environment::Environment()
    : ossInputAvailable(0)
    , ossOutputAvailable(0)
    , pulseOutputAvailable(0)
    , miniAudioOutputAvailable(0)
    , miniAudioCaptureAvailable(0) { }

static const char* audioOutputDriverName(AudioOutputDriverId driver) {
    switch (driver) {
    case AudioOutputDriverAuto:
        return "auto";
    case AudioOutputDriverNull:
        return "null";
    case AudioOutputDriverPulse:
        return "pulse";
    case AudioOutputDriverOss:
        return "oss";
    case AudioOutputDriverMiniAudio:
        return "miniaudio";
    default:
        return "unknown";
    }
}

static int automaticSelectionPrefersMiniAudio() {
#if defined(__APPLE__) || defined(_WIN32) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return 1;
#else
    return 0;
#endif
}

static int appendAudioInputCandidate(AudioInputDriverId* drivers,
    int capacity, int count, AudioInputDriverId driver) {
    if ((drivers != NULL) && (count < capacity))
        drivers[count] = driver;
    return count + 1;
}

static int automaticInputCandidates(const Environment& environment,
    int prefersMiniAudio, AudioInputDriverId* drivers, int capacity) {
    int count = 0;

    if (prefersMiniAudio && environment.miniAudioCaptureAvailable)
        count = appendAudioInputCandidate(drivers, capacity, count,
            AudioInputDriverMiniAudio);

    if (environment.ossInputAvailable)
        count = appendAudioInputCandidate(drivers, capacity, count,
            AudioInputDriverOss);

    if (!prefersMiniAudio && environment.miniAudioCaptureAvailable)
        count = appendAudioInputCandidate(drivers, capacity, count,
            AudioInputDriverMiniAudio);

    return count;
}

static int appendAudioOutputCandidate(AudioOutputDriverId* drivers,
    int capacity, int count, AudioOutputDriverId driver) {
    if ((drivers != NULL) && (count < capacity))
        drivers[count] = driver;
    return count + 1;
}

static int automaticOutputCandidates(const Environment& environment,
    int prefersMiniAudio, AudioOutputDriverId* drivers, int capacity) {
    int count = 0;

    if (prefersMiniAudio && environment.miniAudioOutputAvailable)
        count = appendAudioOutputCandidate(drivers, capacity, count,
            AudioOutputDriverMiniAudio);

    if (environment.pulseOutputAvailable)
        count = appendAudioOutputCandidate(drivers, capacity, count,
            AudioOutputDriverPulse);

    if (!prefersMiniAudio && environment.miniAudioOutputAvailable)
        count = appendAudioOutputCandidate(drivers, capacity, count,
            AudioOutputDriverMiniAudio);

    if (!environment.miniAudioOutputAvailable && environment.ossOutputAvailable)
        count = appendAudioOutputCandidate(drivers, capacity, count,
            AudioOutputDriverOss);

    return count;
}

Environment Environment::detect(const AudioSettings& settings, LogSink& log) {
    Environment environment;
    const char* dspDevicePath = settings.dspDevicePath;

    if (dspDevicePath[0] != '\0') {
        environment.ossInputAvailable = (access(dspDevicePath, R_OK) == 0);
        environment.ossOutputAvailable = (access(dspDevicePath, W_OK) == 0);
    }

#if WITH_PULSE == 1
    environment.pulseOutputAvailable = 1;
#endif

#if WITH_MINIAUDIO == 1
    environment.miniAudioOutputAvailable = 1;
    environment.miniAudioCaptureAvailable = 1;
#endif

    log.debug("runtime environment: dev-dsp=`%s' oss-input=%d oss-output=%d pulse-output=%d miniaudio-output=%d miniaudio-capture=%d\n",
        dspDevicePath, environment.ossInputAvailable,
        environment.ossOutputAvailable, environment.pulseOutputAvailable,
        environment.miniAudioOutputAvailable,
        environment.miniAudioCaptureAvailable);

    return environment;
}

RuntimeFactory::RuntimeFactory(const AudioSettings& settings_,
    const AudioOutputConfig& outputConfig_, const Environment& environment_,
    int visualMaxDimension_, AudioOutputDump* outputDump_,
    RandomSource& randomSource_, SecondsClock& clock_, LogSink& log_)
    : settings(settings_)
    , outputConfig(outputConfig_)
    , environment(environment_)
    , visualMaxDimension(visualMaxDimension_)
    , outputDump(outputDump_)
    , randomSource(randomSource_)
    , clock(clock_)
    , log(log_)
    , pcmSourceFactory(log_) {
    log.debug("runtime factory: created with audio-input-mode=%d sound-dsp-method=%d silent=%d output-driver=%s visual-max-dimension=%d oss-input=%d oss-output=%d pulse-output=%d miniaudio-output=%d miniaudio-capture=%d pulse-server=`%s'\n",
        settings.audioInputMode, settings.soundDSPMethod, settings.silent,
        audioOutputDriverName(outputConfig.outputDriver),
        visualMaxDimension,
        environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable, environment.miniAudioOutputAvailable,
        environment.miniAudioCaptureAvailable,
        outputConfig.pulseServerDisplayName());
    log.debug("runtime factory: output config pulse-latency-ms=%d pulse-target-latency-ms=%d miniaudio-target-latency-ms=%d miniaudio-playback-device=`%s' null-target-latency-ms=%d dsp-target-latency-ms=%d output-dump=`%s'\n",
        outputConfig.pulseLatencyMs, outputConfig.pulseOutputTargetLatencyMs,
        outputConfig.miniAudioOutputTargetLatencyMs,
        outputConfig.miniAudioPlaybackDeviceName.c_str(),
        outputConfig.nullOutputTargetLatencyMs,
        outputConfig.dspOutputTargetLatencyMs,
        outputConfig.outputDumpPath.empty() ? "" : outputConfig.outputDumpPath.c_str());
}

AudioSourceStrategy RuntimeFactory::selectAudioSourceStrategy() const {
    return pcmSourceFactory.selectAudioSourceStrategy(settings);
}

AudioInput* RuntimeFactory::createLineInAudioInputCandidate(
    AudioInputDriverId driver) const {
    PcmSource* source = NULL;

    switch (driver) {
    case AudioInputDriverMiniAudio:
        if (!environment.miniAudioCaptureAvailable) {
            log.debug("    audio input strategy: skipping miniaudio capture because support is unavailable\n");
            return NULL;
        }
        log.debug("    audio input strategy: trying miniaudio capture input\n");
        source = pcmSourceFactory.createMiniAudioCapture(settings);
        break;

    case AudioInputDriverOss:
        if (!environment.ossInputAvailable) {
            log.debug("    audio input strategy: skipping OSS DSP input because it is unavailable\n");
            return NULL;
        }
        log.debug("    audio input strategy: trying OSS DSP input from %s source\n",
            PcmSourceFactory::strategyName(ASS_LineIn));
        source = pcmSourceFactory.create(settings, visualMaxDimension,
            randomSource);
        break;
    }

    if (source == NULL)
        return NULL;

    std::unique_ptr<PcmSource> sourceHolder(source);
    if (sourceHolder->hasError()) {
        log.debug("    audio input strategy: candidate input source failed driver=%d\n",
            driver);
        return NULL;
    }

    log.debug("    audio input strategy: selected AudioInput driver=%d\n",
        driver);
    return new AudioInput(sourceHolder.release(), log, 1,
        settings.inputLoopEnabled);
}

AudioInput* RuntimeFactory::createAudioInput() const {
    AudioSourceStrategy sourceStrategy = selectAudioSourceStrategy();

    log.debug("    audio input strategy: selecting AudioInput for audio-input-mode=%d\n",
        settings.audioInputMode);

    switch (settings.audioInputMode) {
    case AIM_DSPIn: {
        AudioInputDriverId candidates[2];
        int candidateCount = automaticInputCandidates(environment,
            automaticSelectionPrefersMiniAudio(), candidates, 2);
        log.debug("    audio input strategy: line-in candidate count=%d prefers-miniaudio=%d\n",
            candidateCount, automaticSelectionPrefersMiniAudio());
        for (int i = 0; i < candidateCount && i < 2; i++) {
            AudioInput* input = createLineInAudioInputCandidate(candidates[i]);
            if (input != NULL)
                return input;
        }
        log.debug("    audio input strategy: no live line-in PCM source opened\n");
        return NULL;
    }

    case AIM_Random:
    case AIM_File:
        log.debug("    audio input strategy: native PCM input from %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
        break;

    case AIM_None:
        log.debug("    audio input strategy: no PCM input requested\n");
        return NULL;

    default:
        log.debug("    audio input strategy: none, because requested device %d is illegal\n",
            settings.audioInputMode);
        log.debug("    audio input strategy: illegal native AudioInput request %d\n",
            settings.audioInputMode);
        return NULL;
    }

    PcmSource* source = pcmSourceFactory.create(settings, visualMaxDimension,
        randomSource);
    if (source != NULL) {
        log.debug("    audio input strategy: selected AudioInput with source strategy=%s\n",
            PcmSourceFactory::strategyName(sourceStrategy));
        return new AudioInput(source, log, 1, settings.inputLoopEnabled);
    }
    if (settings.audioInputMode == AIM_File) {
        log.debug("    audio input strategy: no native PCM source for %s file source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
    } else {
        log.debug("    audio input strategy: no native PCM source for %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
    }
    log.debug("    audio input strategy: no native AudioInput for source strategy=%s\n",
        PcmSourceFactory::strategyName(sourceStrategy));
    return NULL;
}

AudioOutput* RuntimeFactory::createAudioOutputCandidate(AudioOutputDriverId driver,
    const PcmFormat& format) const {
    switch (driver) {
    case AudioOutputDriverMiniAudio:
#if WITH_MINIAUDIO == 1
        if (environment.miniAudioOutputAvailable) {
            log.debug("    audio output strategy: trying miniaudio output\n");
            AudioMiniAudioOutput* mini = new AudioMiniAudioOutput(format, clock,
                log, outputConfig, outputDump);
            if (mini->isOpen()) {
                log.debug("    audio output strategy: selected AudioMiniAudioOutput\n");
                return mini;
            }
            delete mini;
        } else {
            log.debug("    audio output strategy: skipping miniaudio output because support is unavailable\n");
        }
#else
        log.debug("    audio output strategy: skipping miniaudio output because support is not compiled in\n");
#endif
        return NULL;

    case AudioOutputDriverPulse:
        if (environment.pulseOutputAvailable) {
            log.debug("    audio output strategy: trying Pulse output on server `%s'\n",
                outputConfig.pulseServerDisplayName());
            AudioPulseOutput* pulse = new AudioPulseOutput(format, clock, log,
                outputConfig, outputDump);
            if (pulse->isOpen()) {
                log.debug("    audio output strategy: selected Pulse output\n");
                log.debug("    audio output strategy: selected AudioPulseOutput server=`%s'\n",
                    outputConfig.pulseServerDisplayName());
                return pulse;
            }
            delete pulse;
        } else {
            log.debug("    audio output strategy: skipping Pulse output because support is unavailable\n");
        }
        return NULL;

    case AudioOutputDriverOss:
        if (environment.ossOutputAvailable) {
            log.debug("    audio output strategy: trying OSS DSP output with method %d\n",
                settings.soundDSPMethod);
            AudioDSPOutput* dsp = new AudioDSPOutput(settings, outputConfig,
                visualMaxDimension, clock, log, outputDump);
            if (dsp->isOpen()) {
                log.debug("    audio output strategy: selected AudioDSPOutput method=%d\n",
                    settings.soundDSPMethod);
                return dsp;
            }
            delete dsp;
        } else {
            log.debug("    audio output strategy: skipping OSS DSP output because it is unavailable\n");
        }
        return NULL;

    default:
        return NULL;
    }
}

AudioOutput* RuntimeFactory::createAudioOutput(const PcmFormat& format) const {
    AudioOutputDriverId candidates[3];
    int candidateCount = 0;

    log.debug("    audio output strategy: selecting AudioOutput silent=%d output-driver=%s pulse-output=%d miniaudio-output=%d oss-output=%d\n",
        settings.silent, audioOutputDriverName(outputConfig.outputDriver),
        environment.pulseOutputAvailable, environment.miniAudioOutputAvailable,
        environment.ossOutputAvailable);

    if (settings.silent || outputConfig.outputDriver == AudioOutputDriverNull) {
        log.debug("    audio output strategy: null output, because playback is silent\n");
        log.debug("    audio output strategy: selected AudioNullOutput\n");
        return new AudioNullOutput(clock, log, outputConfig, outputDump);
    }

    if (outputConfig.outputDriver != AudioOutputDriverAuto) {
        AudioOutput* explicitOutput = createAudioOutputCandidate(
            outputConfig.outputDriver, format);
        if (explicitOutput != NULL)
            return explicitOutput;
        log.debug("    audio output strategy: null output, because explicit %s did not open\n",
            audioOutputDriverName(outputConfig.outputDriver));
        log.debug("    audio output strategy: selected AudioNullOutput fallback\n");
        return new AudioNullOutput(clock, log, outputConfig, outputDump);
    }

    candidateCount = automaticOutputCandidates(environment,
        automaticSelectionPrefersMiniAudio(), candidates, 3);
    log.debug("    audio output strategy: automatic candidate count=%d prefers-miniaudio=%d\n",
        candidateCount, automaticSelectionPrefersMiniAudio());
    for (int i = 0; i < candidateCount && i < 3; i++) {
        AudioOutput* candidate = createAudioOutputCandidate(candidates[i], format);
        if (candidate != NULL)
            return candidate;
    }
    if (environment.miniAudioOutputAvailable) {
        log.debug("    audio output strategy: skipping OSS DSP output in automatic mode because miniaudio support is compiled in\n");
    }

    log.debug("    audio output strategy: null output, because no real output opened\n");
    log.debug("    audio output strategy: selected AudioNullOutput fallback\n");
    return new AudioNullOutput(clock, log, outputConfig, outputDump);
}

#ifdef RUNTIME_FACTORY_TEST_HOOKS
int RuntimeFactory::testAutomaticOutputCandidates(const Environment& environment,
    int prefersMiniAudio, AudioOutputDriverId* drivers, int capacity) {
    return automaticOutputCandidates(environment, prefersMiniAudio, drivers,
        capacity);
}

int RuntimeFactory::testAutomaticInputCandidates(const Environment& environment,
    int prefersMiniAudio, AudioInputDriverId* drivers, int capacity) {
    return automaticInputCandidates(environment, prefersMiniAudio, drivers,
        capacity);
}
#endif
