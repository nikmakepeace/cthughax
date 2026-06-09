/** @file
 * Unit coverage for injected audio output dump writing.
 */

#include "Audio.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

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
    static const int kDebugLevel = 5;
    int debugEnabledValue;

    FakeLogSink()
        : debugEnabledValue(0) { }

    virtual int enabled(int level) const {
        return debugEnabledValue && (level == kDebugLevel);
    }
};

static PcmFormat mono16Format() {
    PcmFormat format;
    format.sampleRate = 1000;
    format.channels = 1;
    format.sampleFormat = SF_s16_le;
    return format;
}

static unsigned int readLe32(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8)
        | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static void testInjectedDumpWritesSubmittedPcmAsWav() {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/tmp/cthughanix-output-dump-%ld.wav",
        (long)getpid());
    unlink(path);

    AudioOutputConfig config;
    FakeSecondsClock clock;
    FakeLogSink log;
    AudioOutputDump dump(path, log);
    AudioNullOutput output(clock, log, config, &dump);
    DecodedAudioHistory history(16, mono16Format(), 8, log);
    AudioOutputStream stream(history);
    char scratch[8];
    const char pcm[4] = { 1, 0, 2, 0 };

    assert(history.appendDecodedPcm(pcm, 2) == 2);
    output.configureTiming(1000, mono16Format().bytesPerSample(), 2);
    assert(output.service(stream, scratch, 2, 0) == 1);
    dump.close();

    FILE* file = fopen(path, "rb");
    assert(file != NULL);
    unsigned char bytes[48];
    assert(fread(bytes, 1, sizeof(bytes), file) == sizeof(bytes));
    fclose(file);
    unlink(path);

    assert(bytes[0] == 'R');
    assert(bytes[1] == 'I');
    assert(bytes[2] == 'F');
    assert(bytes[3] == 'F');
    assert(bytes[8] == 'W');
    assert(bytes[9] == 'A');
    assert(bytes[10] == 'V');
    assert(bytes[11] == 'E');
    assert(readLe32(bytes + 40) == 4);
    assert(bytes[44] == 1);
    assert(bytes[45] == 0);
    assert(bytes[46] == 2);
    assert(bytes[47] == 0);
}

static void testSubmittedPcmDebugReporterIsInstanceLocal() {
    AudioSubmittedPcmDebugReporter first;
    AudioSubmittedPcmDebugReporter second;
    const char pcm[4] = { 1, 0, 2, 0 };
    PcmFormat format = mono16Format();
    FakeLogSink log;

    log.debugEnabledValue = 1;
    for (int i = 0; i < 10; i++)
        first.submittedPcm(format, pcm, 2, 4, 4, 0, 2, log);
    assert(first.reportCount() == 8);
    assert(second.reportCount() == 0);

    second.submittedPcm(format, pcm, 2, 4, 4, 0, 2, log);
    assert(second.reportCount() == 1);
}

int main() {
    testInjectedDumpWritesSubmittedPcmAsWav();
    testSubmittedPcmDebugReporterIsInstanceLocal();
    return 0;
}
