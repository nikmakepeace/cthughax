/** @file
 * Unit coverage for decoded PCM history and visual-frame reconstruction.
 */

#include "Audio.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

int audioChannels() { return 2; }
int audioSampleFormat() { return SF_s8; }
int audioBytesPerSample() { return 2; }

static void testHistoryKeepsRecentWindowWhileAppending() {
    DecodedAudioHistory history(8, 1, 4);
    char first[6] = { 0, 1, 2, 3, 4, 5 };
    char second[4] = { 6, 7, 8, 9 };
    char out[8] = { 0 };

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
    DecodedAudioHistory history(4096, 2, 2048);
    AudioFrameBuilder builder;
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

int main() {
    testHistoryKeepsRecentWindowWhileAppending();
    testFrameBuilderReadsCenteredHistory();
    return 0;
}
