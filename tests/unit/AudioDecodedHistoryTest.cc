/** @file
 * Unit coverage for decoded PCM history and visual-frame reconstruction.
 */

#include "Audio.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeLogSink : public LogSink {
protected:
    virtual void write(int, const char*, int, const char*, va_list) { }

public:
    virtual int enabled(int) const { return 0; }
};

static PcmFormat formatFor(int sampleRate, int channels, int sampleFormat) {
    PcmFormat format;
    format.sampleRate = sampleRate;
    format.channels = channels;
    format.sampleFormat = sampleFormat;
    return format;
}

static void testHistoryKeepsRecentWindowWhileAppending() {
    PcmFormat format = formatFor(1000, 1, SF_s8);
    FakeLogSink log;
    DecodedAudioHistory history(8, format, 4, log);
    char first[6] = { 0, 1, 2, 3, 4, 5 };
    char second[4] = { 6, 7, 8, 9 };
    char out[8] = { 0 };

    assert(history.format().sampleRate == format.sampleRate);
    assert(history.format().channels == format.channels);
    assert(history.format().sampleFormat == format.sampleFormat);
    assert(history.appendDecodedPcm(first, 6) == 6);
    assert(history.decodedEndPosition() == 6);
    assert(history.oldestAvailablePosition() == 2);
    assert(history.readPcmAt(2, out, 4) == 4);
    assert(memcmp(out, first + 2, 4) == 0);

    assert(history.appendDecodedPcm(second, 4) == 4);
    assert(history.decodedEndPosition() == 10);
    assert(history.oldestAvailablePosition() == 6);
    assert(history.readPcmAt(0, out, 4) == 0);
    assert(history.readPcmAt(6, out, 4) == 4);
    assert(memcmp(out, second, 4) == 0);
}

static void testFrameBuilderReadsCenteredHistory() {
    FakeLogSink log;
    DecodedAudioHistory history(4096, formatFor(1000, 2, SF_s8), 2048, log);
    AudioFrameBuilder builder(log);
    AudioFrame frame;
    char pcm[2048];

    for (int i = 0; i < 1024; i++) {
        pcm[i * 2] = char(i % 127);
        pcm[i * 2 + 1] = char((i + 1) % 127);
    }

    assert(history.appendDecodedPcm(pcm, 1024) == 1024);
    builder.build(frame, history, 512);

    assert(frame.centerSample == 512);
    assert(frame.samples == 1024);
}

static void testFrameBuilderPreservesSigned8BoundarySamples() {
    FakeLogSink log;
    DecodedAudioHistory history(4096, formatFor(1000, 2, SF_s8), 2048, log);
    AudioFrameBuilder builder(log);
    AudioFrame frame;
    AudioByte pcm[2048];

    memset(pcm, 0, sizeof(pcm));
    pcm[0] = 0x80;
    pcm[1] = 0xff;
    pcm[2] = 0x00;
    pcm[3] = 0x7f;

    assert(history.appendDecodedPcm(reinterpret_cast<const char*>(pcm), 1024) == 1024);
    builder.build(frame, history, 512);

    assert(int(frame.raw[0][1]) == -128);
    assert(int(frame.raw[0][0]) == -1);
    assert(int(frame.raw[1][1]) == 0);
    assert(int(frame.raw[1][0]) == 127);
}

int main() {
    testHistoryKeepsRecentWindowWhileAppending();
    testFrameBuilderReadsCenteredHistory();
    testFrameBuilderPreservesSigned8BoundarySamples();
    return 0;
}
