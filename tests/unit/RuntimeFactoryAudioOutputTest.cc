/** @file
 * Unit coverage for RuntimeFactory audio-output selection fallbacks.
 */

#include "Audio.h"
#include "RuntimeFactory.h"
#include "ProcessServices.h"
#include "config.h"

#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
const char* audioSampleFormatText(int) { return "signed-16"; }

class FakeRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) { return 0; }
};

class FakeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const { return 0.0; }
};

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const { return 0; }
};

class CapturingLogSink : public LogSink {
    char text[8192];

protected:
    virtual void write(int, const char*, int, const char* fmt, va_list args) {
        size_t used = strlen(text);
        if (used >= sizeof(text) - 1)
            return;
        vsnprintf(text + used, sizeof(text) - used, fmt, args);
    }

public:
    CapturingLogSink() { text[0] = '\0'; }

    virtual int enabled(int) const { return 1; }

    int contains(const char* token) const {
        return strstr(text, token) != NULL;
    }
};

static PcmFormat stereo16Format() {
    PcmFormat format;
    format.sampleRate = 48000;
    format.channels = 2;
    format.sampleFormat = SF_s16_le;
    return format;
}

static AudioSettings audioSettings() {
    AudioSettings settings;
    settings.pcmFormat = stereo16Format();
    settings.audioInputMode = AIM_None;
    return settings;
}

static PcmFormat invalidRateFormat() {
    PcmFormat format = stereo16Format();
    format.sampleRate = 0;
    return format;
}

static Environment allUnavailableEnvironment() {
    return Environment();
}

static Environment allAvailableEnvironment() {
    Environment environment;
    environment.ossInputAvailable = 1;
    environment.ossOutputAvailable = 1;
    environment.pulseOutputAvailable = 1;
    environment.miniAudioOutputAvailable = 1;
    environment.miniAudioCaptureAvailable = 1;
    return environment;
}

static std::unique_ptr<AudioOutput> createOutput(const AudioSettings& settings,
    const AudioOutputConfig& config, const Environment& environment) {
    FakeRandomSource random;
    FakeSecondsClock clock;
    FakeLogSink log;
    RuntimeFactory factory(settings, config, environment, 256, NULL, random,
        clock, log);
    return std::unique_ptr<AudioOutput>(
        factory.createAudioOutput(settings.pcmFormat));
}

static std::unique_ptr<AudioOutput> createOutputWithLog(
    const AudioSettings& settings, const AudioOutputConfig& config,
    const Environment& environment, LogSink& log) {
    FakeRandomSource random;
    FakeSecondsClock clock;
    RuntimeFactory factory(settings, config, environment, 256, NULL, random,
        clock, log);
    return std::unique_ptr<AudioOutput>(
        factory.createAudioOutput(settings.pcmFormat));
}

static void assertNullOutput(AudioOutput* output) {
    assert(output != NULL);
    assert(dynamic_cast<AudioNullOutput*>(output) != NULL);
    assert(output->isOpen());
    assert(!output->isRealtime());
}

static void assertAutomaticCandidates(const Environment& environment,
    int prefersMiniAudio, const AudioOutputDriverId* expected,
    int expectedCount) {
    AudioOutputDriverId actual[3] = {
        AudioOutputDriverNull,
        AudioOutputDriverNull,
        AudioOutputDriverNull
    };

    int count = RuntimeFactory::testAutomaticOutputCandidates(environment,
        prefersMiniAudio, actual, 3);
    assert(count == expectedCount);
    for (int i = 0; i < expectedCount; i++)
        assert(actual[i] == expected[i]);
}

static void assertAutomaticInputCandidates(const Environment& environment,
    int prefersMiniAudio, const AudioInputDriverId* expected,
    int expectedCount) {
    AudioInputDriverId actual[2] = {
        AudioInputDriverOss,
        AudioInputDriverOss
    };

    int count = RuntimeFactory::testAutomaticInputCandidates(environment,
        prefersMiniAudio, actual, 2);
    assert(count == expectedCount);
    for (int i = 0; i < expectedCount; i++)
        assert(actual[i] == expected[i]);
}

static void testAutomaticLinuxOrderKeepsPulseBeforeMiniAudio() {
    Environment environment = allAvailableEnvironment();
    AudioOutputDriverId expected[] = {
        AudioOutputDriverPulse,
        AudioOutputDriverMiniAudio
    };

    assertAutomaticCandidates(environment, 0, expected, 2);
}

static void testAutomaticMacWindowsOrderPrefersMiniAudio() {
    Environment environment = allAvailableEnvironment();
    AudioOutputDriverId expected[] = {
        AudioOutputDriverMiniAudio,
        AudioOutputDriverPulse
    };

    assertAutomaticCandidates(environment, 1, expected, 2);
}

static void testAutomaticOssFallbackRequiresMissingMiniAudio() {
    Environment environment = allAvailableEnvironment();
    AudioOutputDriverId expectedWithMini[] = {
        AudioOutputDriverPulse,
        AudioOutputDriverMiniAudio
    };
    assertAutomaticCandidates(environment, 0, expectedWithMini, 2);

    environment.miniAudioOutputAvailable = 0;
    AudioOutputDriverId expectedWithoutMini[] = {
        AudioOutputDriverPulse,
        AudioOutputDriverOss
    };
    assertAutomaticCandidates(environment, 0, expectedWithoutMini, 2);
}

static void testAutomaticOssOnlyWhenItIsOnlyLegacyOutput() {
    Environment environment = allUnavailableEnvironment();
    environment.ossOutputAvailable = 1;
    AudioOutputDriverId expected[] = {
        AudioOutputDriverOss
    };

    assertAutomaticCandidates(environment, 0, expected, 1);
    assertAutomaticCandidates(environment, 1, expected, 1);
}

static void testSilentOverridesExplicitRealDriver() {
    AudioSettings settings = audioSettings();
    settings.silent = 1;
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverMiniAudio;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allAvailableEnvironment());

    assertNullOutput(output.get());
}

static void testExplicitNullSelectsNullOutput() {
    AudioSettings settings = audioSettings();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverNull;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allAvailableEnvironment());

    assertNullOutput(output.get());
}

static void testExplicitUnavailableMiniAudioFallsBackToNull() {
    AudioSettings settings = audioSettings();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverMiniAudio;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allUnavailableEnvironment());

    assertNullOutput(output.get());
}

static void testExplicitUnavailablePulseFallsBackToNull() {
    AudioSettings settings = audioSettings();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverPulse;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allUnavailableEnvironment());

    assertNullOutput(output.get());
}

static void testExplicitUnavailableOssFallsBackToNull() {
    AudioSettings settings = audioSettings();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverOss;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allUnavailableEnvironment());

    assertNullOutput(output.get());
}

static void testAutoWithNoBackendsFallsBackToNull() {
    AudioSettings settings = audioSettings();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverAuto;

    std::unique_ptr<AudioOutput> output = createOutput(settings, config,
        allUnavailableEnvironment());

    assertNullOutput(output.get());
}

static void testExplicitMiniAudioOpenFailureLogsFallbackReason() {
#if WITH_MINIAUDIO == 1
    AudioSettings settings = audioSettings();
    settings.pcmFormat = invalidRateFormat();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverMiniAudio;
    Environment environment = allUnavailableEnvironment();
    environment.miniAudioOutputAvailable = 1;
    CapturingLogSink log;

    std::unique_ptr<AudioOutput> output = createOutputWithLog(settings,
        config, environment, log);

    assertNullOutput(output.get());
    assert(log.contains("trying miniaudio output"));
    assert(log.contains("miniaudio unavailable for invalid PCM format"));
    assert(log.contains("null output, because explicit miniaudio did not open"));
    assert(log.contains("selected AudioNullOutput fallback"));
#endif
}

static void testAutomaticMiniAudioOpenFailureLogsFallbackReason() {
#if WITH_MINIAUDIO == 1
    AudioSettings settings = audioSettings();
    settings.pcmFormat = invalidRateFormat();
    AudioOutputConfig config;
    config.outputDriver = AudioOutputDriverAuto;
    Environment environment = allUnavailableEnvironment();
    environment.miniAudioOutputAvailable = 1;
    CapturingLogSink log;

    std::unique_ptr<AudioOutput> output = createOutputWithLog(settings,
        config, environment, log);

    assertNullOutput(output.get());
    assert(log.contains("automatic candidate count=1"));
    assert(log.contains("trying miniaudio output"));
    assert(log.contains("miniaudio unavailable for invalid PCM format"));
    assert(log.contains("null output, because no real output opened"));
    assert(log.contains("selected AudioNullOutput fallback"));
#endif
}

static void testAutomaticLinuxLineInKeepsOssBeforeMiniAudio() {
    Environment environment = allAvailableEnvironment();
    AudioInputDriverId expected[] = {
        AudioInputDriverOss,
        AudioInputDriverMiniAudio
    };

    assertAutomaticInputCandidates(environment, 0, expected, 2);
}

static void testAutomaticMacWindowsLineInPrefersMiniAudio() {
    Environment environment = allAvailableEnvironment();
    AudioInputDriverId expected[] = {
        AudioInputDriverMiniAudio,
        AudioInputDriverOss
    };

    assertAutomaticInputCandidates(environment, 1, expected, 2);
}

static void testAutomaticLineInUsesMiniAudioWithoutOss() {
    Environment environment = allUnavailableEnvironment();
    environment.miniAudioCaptureAvailable = 1;
    AudioInputDriverId expected[] = {
        AudioInputDriverMiniAudio
    };

    assertAutomaticInputCandidates(environment, 0, expected, 1);
    assertAutomaticInputCandidates(environment, 1, expected, 1);
}

int main() {
    testAutomaticLinuxOrderKeepsPulseBeforeMiniAudio();
    testAutomaticMacWindowsOrderPrefersMiniAudio();
    testAutomaticOssFallbackRequiresMissingMiniAudio();
    testAutomaticOssOnlyWhenItIsOnlyLegacyOutput();
    testAutomaticLinuxLineInKeepsOssBeforeMiniAudio();
    testAutomaticMacWindowsLineInPrefersMiniAudio();
    testAutomaticLineInUsesMiniAudioWithoutOss();
    testSilentOverridesExplicitRealDriver();
    testExplicitNullSelectsNullOutput();
    testExplicitUnavailableMiniAudioFallsBackToNull();
    testExplicitUnavailablePulseFallsBackToNull();
    testExplicitUnavailableOssFallsBackToNull();
    testAutoWithNoBackendsFallsBackToNull();
    testExplicitMiniAudioOpenFailureLogsFallbackReason();
    testAutomaticMiniAudioOpenFailureLogsFallbackReason();
    return 0;
}
