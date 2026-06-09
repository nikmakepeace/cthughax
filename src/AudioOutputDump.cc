/** @file
 * Optional WAV dump writer for submitted output PCM.
 */

#include "AudioOutputDump.h"
#include "AudioOptions.h"
#include "ProcessServices.h"

#include <errno.h>

static int audioOutputDumpClose(FILE*& stream) {
    int ret = fclose(stream);
    stream = NULL;
    return ret;
}

static void audioOutputDumpWriteLe16(FILE* out, unsigned int value) {
    fputc(value & 255, out);
    fputc((value >> 8) & 255, out);
}

static void audioOutputDumpWriteLe32(FILE* out, unsigned int value) {
    fputc(value & 255, out);
    fputc((value >> 8) & 255, out);
    fputc((value >> 16) & 255, out);
    fputc((value >> 24) & 255, out);
}

static void audioOutputDumpWriteHeader(FILE* out, unsigned int dataBytes,
    int sampleRate, int channels, int bitsPerSample) {
    unsigned int blockAlign = (unsigned int)((channels * bitsPerSample) / 8);
    unsigned int byteRate = (unsigned int)(sampleRate * blockAlign);
    unsigned int riffBytes = 36 + dataBytes;

    fwrite("RIFF", 1, 4, out);
    audioOutputDumpWriteLe32(out, riffBytes);
    fwrite("WAVE", 1, 4, out);
    fwrite("fmt ", 1, 4, out);
    audioOutputDumpWriteLe32(out, 16);
    audioOutputDumpWriteLe16(out, 1);
    audioOutputDumpWriteLe16(out, (unsigned int)channels);
    audioOutputDumpWriteLe32(out, (unsigned int)sampleRate);
    audioOutputDumpWriteLe32(out, byteRate);
    audioOutputDumpWriteLe16(out, blockAlign);
    audioOutputDumpWriteLe16(out, (unsigned int)bitsPerSample);
    fwrite("data", 1, 4, out);
    audioOutputDumpWriteLe32(out, dataBytes);
}

AudioOutputDump::AudioOutputDump(const std::string& path, LogSink& log_)
    : pathValue()
    , log(log_)
    , out(NULL)
    , initialized(0)
    , enabled(0)
    , sampleRate(0)
    , channels(0)
    , bitsPerSample(0)
    , bytesWritten(0) {
    configure(path);
}

AudioOutputDump::~AudioOutputDump() {
    close();
}

void AudioOutputDump::configure(const std::string& path) {
    close();
    pathValue = path;
    initialized = 0;
    enabled = 0;
    sampleRate = 0;
    channels = 0;
    bitsPerSample = 0;
    bytesWritten = 0;
}

void AudioOutputDump::close() {
    if (out != NULL)
        audioOutputDumpClose(out);
}

const std::string& AudioOutputDump::path() const {
    return pathValue;
}

int AudioOutputDump::bitsPerSampleForFormat(const PcmFormat& format) const {
    switch (format.sampleFormat) {
    case SF_u8:
        return 8;
    case SF_s16_le:
    case SF_s16_be:
        return 16;
    default:
        return 0;
    }
}

void AudioOutputDump::initialize(const PcmFormat& format) {
    if (initialized)
        return;

    initialized = 1;
    enabled = !pathValue.empty();
    if (!enabled)
        return;

    sampleRate = format.sampleRate;
    channels = format.channels;
    bitsPerSample = bitsPerSampleForFormat(format);

    if ((sampleRate <= 0) || (channels <= 0) || (bitsPerSample == 0)) {
        log.warn("Can not create audio output dump `%s' for format `%s'.\n",
            pathValue.c_str(), audioSampleFormatText(format.sampleFormat));
        enabled = 0;
        return;
    }

    out = fopen(pathValue.c_str(), "wb+");
    if (out == NULL) {
        log.errorErrno(errno, "Can not create audio output dump `%s'.",
            pathValue.c_str());
        enabled = 0;
        return;
    }

    audioOutputDumpWriteHeader(out, 0, sampleRate, channels, bitsPerSample);
    log.debug("    audio output: dumping submitted PCM to `%s' rate=%d channels=%d bits=%d format=%s\n",
        pathValue.c_str(), sampleRate, channels, bitsPerSample,
        audioSampleFormatText(format.sampleFormat));
}

void AudioOutputDump::refreshHeader() {
    if (out == NULL)
        return;

    long pos = ftell(out);
    unsigned int dataBytes = (bytesWritten > 0x7fffffffLL)
        ? 0x7fffffffU : (unsigned int)bytesWritten;

    fseek(out, 0, SEEK_SET);
    audioOutputDumpWriteHeader(out, dataBytes, sampleRate, channels, bitsPerSample);
    fseek(out, pos, SEEK_SET);
}

void AudioOutputDump::append(const PcmFormat& format, const char* data,
    int bytes) {
    initialize(format);

    if (!enabled || (out == NULL) || (data == NULL) || (bytes <= 0))
        return;

    if (format.sampleFormat == SF_s16_be) {
        for (int i = 0; i + 1 < bytes; i += 2) {
            fputc((unsigned char)data[i + 1], out);
            fputc((unsigned char)data[i], out);
        }
    } else {
        fwrite(data, 1, bytes, out);
    }

    bytesWritten += bytes;
    refreshHeader();
    fflush(out);
}
