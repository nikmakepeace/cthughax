/** @file
 * Startup-time runtime composition helpers.
 */

#include "config.h"
#include "RuntimeFactory.h"
#include "ProcessServices.h"

#include <unistd.h>

Environment::Environment()
    : ossInputAvailable(0)
    , ossOutputAvailable(0)
    , pulseOutputAvailable(0) { }

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

    log.debug("runtime environment: dev-dsp=`%s' oss-input=%d oss-output=%d pulse-output=%d\n",
        dspDevicePath, environment.ossInputAvailable,
        environment.ossOutputAvailable, environment.pulseOutputAvailable);

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
    log.debug("runtime factory: created with audio-input-mode=%d sound-dsp-method=%d silent=%d visual-max-dimension=%d oss-input=%d oss-output=%d pulse-output=%d pulse-server=`%s'\n",
        settings.audioInputMode, settings.soundDSPMethod, settings.silent,
        visualMaxDimension,
        environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable,
        outputConfig.pulseServerDisplayName());
    log.debug("runtime factory: output config pulse-latency-ms=%d pulse-target-latency-ms=%d null-target-latency-ms=%d dsp-target-latency-ms=%d output-dump=`%s'\n",
        outputConfig.pulseLatencyMs, outputConfig.pulseOutputTargetLatencyMs,
        outputConfig.nullOutputTargetLatencyMs,
        outputConfig.dspOutputTargetLatencyMs,
        outputConfig.outputDumpPath.empty() ? "" : outputConfig.outputDumpPath.c_str());
}

AudioSourceStrategy RuntimeFactory::selectAudioSourceStrategy() const {
    return pcmSourceFactory.selectAudioSourceStrategy(settings);
}

AudioInput* RuntimeFactory::createAudioInput() const {
    AudioSourceStrategy sourceStrategy = selectAudioSourceStrategy();

    log.debug("    audio input strategy: selecting AudioInput for audio-input-mode=%d\n",
        settings.audioInputMode);

    switch (settings.audioInputMode) {
    case AIM_DSPIn:
        if (!environment.ossInputAvailable) {
            log.debug("    audio input strategy: OSS DSP input unavailable; no PCM source\n");
            return NULL;
        }
        log.debug("    audio input strategy: native OSS DSP input from %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
        break;

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

AudioOutput* RuntimeFactory::createAudioOutput(const PcmFormat& format) const {
    log.debug("    audio output strategy: selecting AudioOutput silent=%d pulse-output=%d oss-output=%d\n",
        settings.silent, environment.pulseOutputAvailable, environment.ossOutputAvailable);

    if (settings.silent) {
        log.debug("    audio output strategy: null output, because playback is silent\n");
        log.debug("    audio output strategy: selected AudioNullOutput\n");
        return new AudioNullOutput(clock, log, outputConfig, outputDump);
    }

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

    log.debug("    audio output strategy: null output, because no real output opened\n");
    log.debug("    audio output strategy: selected AudioNullOutput fallback\n");
    return new AudioNullOutput(clock, log, outputConfig, outputDump);
}
