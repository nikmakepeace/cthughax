/** @file
 * Unit coverage for explicit audio output config propagation.
 */

#include "Audio.h"
#include "AudioSettings.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
const char* audioSampleFormatText(int) { return "signed-16"; }

class FakeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

static PcmFormat stereo16Format() {
    PcmFormat format;
    format.sampleRate = 48000;
    format.channels = 2;
    format.sampleFormat = SF_s16_le;
    return format;
}

static void testNullOutputReceivesTargetLatency() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    config.nullOutputTargetLatencyMs = 17;
    AudioNullOutput output(clock, log, config);

    assert(output.targetLatencyMs() == 17);
    output.configureTiming(1000, 1, 1);
    assert(output.targetDelaySamples() == 17);
}

static void testPulseOutputReceivesServerAndLatencyWithoutOpening() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    config.pulseServer = "pulse.local";
    config.pulseLatencyMs = 1234;
    config.pulseOutputTargetLatencyMs = 23;
    AudioPulseOutput output(stereo16Format(), clock, log, config, NULL, 0);

    assert(strcmp(output.serverName(), "pulse.local") == 0);
    assert(strcmp(output.serverDisplayName(), "pulse.local") == 0);
    assert(output.pulseLatencyMs() == 1234);
    assert(output.targetLatencyMs() == 23);
    output.configureTiming(1000, stereo16Format().bytesPerSample(), 1);
    assert(output.targetDelaySamples() == 23);
    assert(output.queuedTargetSamples() == 23);

    output.pulseUnderflow();
    assert(output.underflowCount() == 1);
    assert(output.queuedTargetSamples() == 73);
}

static void testPulseUnderflowCountIsInstanceLocal() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputConfig config;
    AudioPulseOutput first(stereo16Format(), clock, log, config, NULL, 0);
    AudioPulseOutput second(stereo16Format(), clock, log, config, NULL, 0);

    assert(first.underflowCount() == 0);
    assert(second.underflowCount() == 0);
    first.pulseUnderflow();
    first.pulseUnderflow();
    assert(first.underflowCount() == 2);
    assert(second.underflowCount() == 0);
}

static void testDspOutputReceivesTargetLatency() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioSettings settings;
    settings.pcmFormat = stereo16Format();
    settings.soundDSPMethod = 0;
    settings.dspFragments = 8;
    settings.dspFragmentSize = 9;
    strncpy(settings.dspDevicePath, "/tmp/cthughanix-no-such-dsp", PATH_MAX);
    settings.dspDevicePath[PATH_MAX - 1] = '\0';

    AudioOutputConfig config;
    config.dspOutputTargetLatencyMs = 29;
    AudioDSPOutput output(settings, config, 256, clock, log);

    assert(!output.isOpen());
    assert(output.targetLatencyMs() == 29);
    output.configureTiming(1000, stereo16Format().bytesPerSample(), 1);
    assert(output.targetDelaySamples() == 29);
}

int main() {
    testNullOutputReceivesTargetLatency();
    testPulseOutputReceivesServerAndLatencyWithoutOpening();
    testPulseUnderflowCountIsInstanceLocal();
    testDspOutputReceivesTargetLatency();
    return 0;
}
