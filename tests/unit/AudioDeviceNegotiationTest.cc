/** @file
 * Unit coverage for source/output-local PCM format negotiation.
 */

#include "Audio.h"
#include "AudioSettings.h"
#include "PcmSourceFactory.h"
#include "ProcessServices.h"
#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

const char* audioSampleFormatText(int) { return "test-format"; }

class FakeRandomSource : public RandomSource {
    int values[16];
    int valueCount;
    int cursor;

public:
    FakeRandomSource()
        : valueCount(0)
        , cursor(0) { }

    void add(int value) {
        assert(valueCount < int(sizeof(values) / sizeof(values[0])));
        values[valueCount++] = value;
    }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;
        int value = (cursor < valueCount) ? values[cursor++] : 0;
        return value % exclusiveMax;
    }
};

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

static std::string fixturePath(const char* fileName) {
    return std::string(CTH_AUDIO_FIXTURE_DIR) + "/" + fileName;
}

static PcmFormat formatFor(int sampleRate, int channels, int sampleFormat) {
    PcmFormat format;
    format.sampleRate = sampleRate;
    format.channels = channels;
    format.sampleFormat = sampleFormat;
    return format;
}

static void assertFormatEquals(const PcmFormat& actual,
    const PcmFormat& expected) {
    assert(actual.sampleRate == expected.sampleRate);
    assert(actual.channels == expected.channels);
    assert(actual.sampleFormat == expected.sampleFormat);
}

static void testWavSourcePublishesHeaderFormat() {
    FakeLogSink log;
    WavPcmSource source(fixturePath("sine-50-1600-doubling-4s.wav").c_str(),
        log);

    assert(!source.hasError());
    assertFormatEquals(source.format(), formatFor(44100, 2, SF_s16_le));
}

static void testMp3SourcePublishesDecoderFormatWhenAvailable() {
    FakeLogSink log;
    Minimp3PcmSource source(fixturePath("prism.mp3").c_str(), log);

    if (source.hasError())
        return;

    assertFormatEquals(source.format(), formatFor(48000, 2, SF_s16_le));
}

static void testMp3FactoryFollowsConfiguredMinimp3Availability() {
    AudioSettings settings;
    settings.audioInputMode = AIM_File;
    strncpy(settings.fileName, fixturePath("prism.mp3").c_str(), PATH_MAX);
    settings.fileName[PATH_MAX - 1] = '\0';

    FakeLogSink log;
    FakeRandomSource randomSource;
    PcmSourceFactory factory(log);
    PcmSource* source = factory.create(settings, 256, randomSource);

#if WITH_MINIMP3 == 1
    assert(source != NULL);
    assert(!source->hasError());
    assertFormatEquals(source->format(), formatFor(48000, 2, SF_s16_le));
#else
    assert(source == NULL);
#endif

    delete source;
}

static void testRawSourceUsesExplicitFormat() {
    char path[] = "/tmp/cthughanix-raw-format-XXXXXX";
    int fd = mkstemp(path);
    assert(fd != -1);
    FILE* file = fdopen(fd, "wb");
    assert(file != NULL);
    const unsigned char data[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    assert(fwrite(data, 1, sizeof(data), file) == sizeof(data));
    assert(fclose(file) == 0);

    PcmFormat requested = formatFor(22050, 1, SF_u8);
    FakeLogSink log;
    RawPcmSource source(path, requested, log);
    unlink(path);

    assert(!source.hasError());
    assertFormatEquals(source.format(), requested);
}

static void testRandomNoiseKeepsSessionRateAndOwnSampleShape() {
    FakeRandomSource randomSource;
    FakeLogSink log;
    RandomNoisePcmSource source(formatFor(32000, 1, SF_s16_le),
        randomSource, log);

    assert(!source.hasError());
    assertFormatEquals(source.format(), formatFor(32000, 2, SF_u8));
}

static void testRandomNoiseUsesInjectedRandomSource() {
    FakeRandomSource randomSource;
    randomSource.add(200);
    randomSource.add(1);
    randomSource.add(100);
    randomSource.add(1);
    randomSource.add(0);
    randomSource.add(1);
    randomSource.add(255);
    randomSource.add(1);
    FakeLogSink log;
    RandomNoisePcmSource source(formatFor(32000, 1, SF_s16_le),
        randomSource, log);
    unsigned char data[6] = { 0, 0, 0, 0, 0, 0 };

    assert(source.read(reinterpret_cast<char*>(data), sizeof(data), 3) == 3);

    assert(data[0] == 144);
    assert(data[1] == 112);
    assert(data[2] == 145);
    assert(data[3] == 111);
    assert(data[4] == 145);
    assert(data[5] == 111);
}

static void testDspSourceKeepsRequestedFormatLocallyWhenOpenFails() {
    AudioSettings settings;
    settings.pcmFormat = formatFor(48000, 1, SF_s16_le);
    settings.soundDSPMethod = 0;
    settings.dspFragments = 8;
    settings.dspFragmentSize = 9;
    settings.dspSyncEnabled = 1;
    strncpy(settings.dspDevicePath, "/tmp/cthughanix-no-such-dsp", PATH_MAX);
    settings.dspDevicePath[PATH_MAX - 1] = '\0';

    FakeLogSink log;
    DspPcmSource source(settings, 256, log);

    assert(source.hasError());
    assertFormatEquals(source.format(), settings.pcmFormat);
}

static void testDspOutputNegotiatesWithoutGlobalSetters() {
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioSettings settings;
    settings.pcmFormat = formatFor(48000, 2, SF_s16_le);
    settings.soundDSPMethod = 0;
    settings.dspFragments = 8;
    settings.dspFragmentSize = 9;
    strncpy(settings.dspDevicePath, "/tmp/cthughanix-no-such-dsp", PATH_MAX);
    settings.dspDevicePath[PATH_MAX - 1] = '\0';

    AudioOutputConfig outputConfig;
    outputConfig.dspOutputTargetLatencyMs = 37;
    AudioDSPOutput output(settings, outputConfig, 256, clock, log);

    assert(!output.isOpen());
    assert(output.targetLatencyMs() == 37);
}

int main() {
    testWavSourcePublishesHeaderFormat();
    testMp3SourcePublishesDecoderFormatWhenAvailable();
    testMp3FactoryFollowsConfiguredMinimp3Availability();
    testRawSourceUsesExplicitFormat();
    testRandomNoiseKeepsSessionRateAndOwnSampleShape();
    testRandomNoiseUsesInjectedRandomSource();
    testDspSourceKeepsRequestedFormatLocallyWhenOpenFails();
    testDspOutputNegotiatesWithoutGlobalSetters();
    return 0;
}
