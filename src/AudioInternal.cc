// Private helper implementations shared across the audio split units.

#include "cthugha.h"
#include "AudioInternal.h"
#include "Audio.h"
#include "imath.h"

static int readSigned16Le(const unsigned char* p) {
    unsigned int value = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
    return (value & 0x8000) ? (int)value - 0x10000 : (int)value;
}

static int readSigned16Be(const unsigned char* p) {
    unsigned int value = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    return (value & 0x8000) ? (int)value - 0x10000 : (int)value;
}

static unsigned int readUnsigned16Le(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int readUnsigned16Be(const unsigned char* p) {
    return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}

int audioSampleWindowForVisualMaxDimension(int visualMaxDimension) {
    if (visualMaxDimension < 1)
        visualMaxDimension = 160;

    return 1 << ilog2(visualMaxDimension);
}

static int audioPcmPeak(const char* data, int samples) {
    const unsigned char* bytes = (const unsigned char*)data;
    int peak = 0;
    int channels = audioChannels();
    if ((data == NULL) || (samples <= 0) || (channels <= 0))
        return 0;

    switch (audioSampleFormat()) {
    case SF_u8:
        for (int i = 0; i < samples * channels; i++) {
            int sample = (int)bytes[i] - 128;
            peak = max(peak, abs(sample));
        }
        break;
    case SF_s8:
        for (int i = 0; i < samples * channels; i++)
            peak = max(peak, abs((int)((const signed char*)data)[i]));
        break;
    case SF_u16_le:
        for (int i = 0; i < samples * channels; i++) {
            int sample = (int)readUnsigned16Le(bytes + i * 2) - 32768;
            peak = max(peak, abs(sample));
        }
        break;
    case SF_s16_le:
        for (int i = 0; i < samples * channels; i++)
            peak = max(peak, abs(readSigned16Le(bytes + i * 2)));
        break;
    case SF_u16_be:
        for (int i = 0; i < samples * channels; i++) {
            int sample = (int)readUnsigned16Be(bytes + i * 2) - 32768;
            peak = max(peak, abs(sample));
        }
        break;
    case SF_s16_be:
        for (int i = 0; i < samples * channels; i++)
            peak = max(peak, abs(readSigned16Be(bytes + i * 2)));
        break;
    default:
        break;
    }

    return peak;
}

void audioDebugSubmittedPcm(const char* scratch, int samples, int bytes, int written,
    int queuedSamples, long long submittedEndSample) {
    static int reports = 0;

    if (!CTH_LOG_ENABLED(CTH_LOG_DEBUG) || (reports >= 8))
        return;

    reports++;

    CTH_DEBUG("    audio output: submitted samples=%d bytes=%d written=%d peak=%d queued-samples=%d submitted-end-sample=%lld\n",
        samples, bytes, written, audioPcmPeak(scratch, samples), queuedSamples,
        submittedEndSample);
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

static int audioOutputDumpBitsPerSample() {
    switch (audioSampleFormat()) {
    case SF_u8:
        return 8;
    case SF_s16_le:
    case SF_s16_be:
        return 16;
    default:
        return 0;
    }
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

static void audioOutputDumpRefreshHeader(FILE* out, long long bytesWritten,
    int sampleRate, int channels, int bitsPerSample) {
    long pos = ftell(out);
    unsigned int dataBytes = (bytesWritten > 0x7fffffffLL)
        ? 0x7fffffffU : (unsigned int)bytesWritten;

    fseek(out, 0, SEEK_SET);
    audioOutputDumpWriteHeader(out, dataBytes, sampleRate, channels, bitsPerSample);
    fseek(out, pos, SEEK_SET);
}

void audioOutputDumpSubmittedPcm(const char* data, int bytes) {
    static FILE* out = NULL;
    static int initialized = 0;
    static int enabled = 0;
    static int sampleRate = 0;
    static int channels = 0;
    static int bitsPerSample = 0;
    static long long bytesWritten = 0;

    if (!initialized) {
        initialized = 1;
        enabled = audio_output_dump[0] != '\0';
        if (enabled) {
            sampleRate = audioSampleRateHz();
            channels = audioChannels();
            bitsPerSample = audioOutputDumpBitsPerSample();

            if ((sampleRate <= 0) || (channels <= 0) || (bitsPerSample == 0)) {
                CTH_WARN("Can not create audio output dump `%s' for format `%s'.\n",
                    audio_output_dump, audioSampleFormatText());
                enabled = 0;
            }
        }
        if (enabled) {
            out = fopen(audio_output_dump, "wb+");
            if (out == NULL) {
                CTH_ERRNO(errno, "Can not create audio output dump `%s'.",
                    audio_output_dump);
                enabled = 0;
            } else {
                audioOutputDumpWriteHeader(out, 0, sampleRate, channels, bitsPerSample);
                CTH_DEBUG("    audio output: dumping submitted PCM to `%s' rate=%d channels=%d bits=%d format=%s\n",
                    audio_output_dump, sampleRate, channels, bitsPerSample,
                    audioSampleFormatText());
            }
        }
    }

    if (!enabled || (out == NULL) || (data == NULL) || (bytes <= 0))
        return;

    if (audioSampleFormat() == SF_s16_be) {
        for (int i = 0; i + 1 < bytes; i += 2) {
            fputc((unsigned char)data[i + 1], out);
            fputc((unsigned char)data[i], out);
        }
    } else {
        fwrite(data, 1, bytes, out);
    }

    bytesWritten += bytes;
    audioOutputDumpRefreshHeader(out, bytesWritten, sampleRate, channels, bitsPerSample);
    fflush(out);
}
