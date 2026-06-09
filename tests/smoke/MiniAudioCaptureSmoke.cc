/** @file
 * Opt-in miniaudio capture smoke test.
 */

#include "Audio.h"
#include "AudioSettings.h"
#include "ProcessServices.h"

#include <chrono>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <thread>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class SmokeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const { return 0; }
};

int main() {
    SmokeLogSink log;
    AudioSettings settings;
    settings.audioInputMode = AIM_DSPIn;
    settings.pcmFormat.sampleRate = 48000;
    settings.pcmFormat.channels = 2;
    settings.pcmFormat.sampleFormat = SF_s16_le;

    MiniAudioCapturePcmSource source(settings, log);
    if (source.hasError() || !source.isOpen()) {
        if (strcmp(source.backendName(), "Null") == 0) {
            fprintf(stderr, "miniaudio opened the Null backend, not a real capture device\n");
            return 3;
        }
        fprintf(stderr, "miniaudio capture did not open a real input device\n");
        return 2;
    }
    if (strcmp(source.backendName(), "Null") == 0) {
        fprintf(stderr, "miniaudio opened the Null backend, not a real capture device\n");
        return 3;
    }

    const int samples = settings.pcmFormat.sampleRate / 4;
    char buffer[48000];
    int captured = 0;
    for (int i = 0; (i < 20) && (captured <= 0); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        captured = source.read(buffer, sizeof(buffer), samples);
    }

    if (captured <= 0) {
        fprintf(stderr, "miniaudio capture opened but did not produce frames\n");
        return 4;
    }

    return 0;
}
