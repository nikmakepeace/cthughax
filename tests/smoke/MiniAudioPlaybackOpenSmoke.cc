/** @file
 * Opt-in miniaudio default playback-device smoke test.
 */

#include "Audio.h"
#include "ProcessServices.h"
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
const char* audioSampleFormatText(int) { return "signed-16"; }

class SmokeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

class SmokeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const {
        return 0;
    }
};

static PcmFormat smokePlaybackFormat() {
    PcmFormat format;
    format.sampleRate = 48000;
    format.channels = 2;
    format.sampleFormat = SF_s16_le;
    return format;
}

int main() {
#if WITH_MINIAUDIO == 1
    SmokeSecondsClock clock;
    SmokeLogSink log;
    AudioOutputConfig config;
    config.miniAudioOutputTargetLatencyMs = 100;
    PcmFormat format = smokePlaybackFormat();

    AudioMiniAudioOutput output(format, clock, log, config, NULL, 1);
    if (!output.isOpen()) {
        if (strcmp(output.backendName(), "Null") == 0) {
            fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
            return 4;
        }
        fprintf(stderr, "miniaudio default playback device did not open\n");
        return 2;
    }

    if (!output.supportsCallbackDrain()) {
        fprintf(stderr, "miniaudio output opened without callback drain support\n");
        return 3;
    }

    if (strcmp(output.backendName(), "Null") == 0) {
        fprintf(stderr, "miniaudio opened the Null backend, not a real playback device\n");
        return 4;
    }

    return 0;
#else
    fprintf(stderr, "miniaudio support is not compiled in\n");
    return 77;
#endif
}
