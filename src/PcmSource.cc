// File, raw, random PCM sources, plus the AudioInput wrapper.

#include "Audio.h"
#include "imath.h"
#include "ProcessServices.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef CTH_DISABLE_MINIMP3
#undef WITH_MINIMP3
#define WITH_MINIMP3 0
#endif

#if WITH_MINIMP3 == 1
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#ifndef UINT64_MAX
#define UINT64_MAX ((uint64_t)-1)
#endif
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "../external/minimp3/minimp3_ex.h"
#endif

static int pcmSourceClose(FILE*& stream) {
    int ret = fclose(stream);
    stream = NULL;
    return ret;
}

PcmSource::PcmSource(LogSink& log_)
    : log(log_)
    , error(0) { }

PcmSource::~PcmSource() { }

int PcmSource::rawBufferSize(int frameRawSize, int /*samplesRequested*/) const {
    return frameRawSize;
}

static unsigned int readLe16(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int readLe32(const unsigned char* p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16)
        | ((unsigned int)p[3] << 24);
}

AudioInput::AudioInput(PcmSource* source_, LogSink& log_, int takeOwnership,
    int loopEnabled)
    : error(0)
    , log(log_)
    , source(source_)
    , sourceOwned(takeOwnership)
    , loopEnabledValue(loopEnabled ? 1 : 0)
    , finished(0) {
    if ((source == NULL) || source->hasError()) {
        log.debug("audio input: source construction failed\n");
        error = 1;
        return;
    }

    const PcmFormat& format = source->format();
    log.debug("audio input: created format rate=%d channels=%d format=%d loop=%d\n",
        format.sampleRate, format.channels, format.sampleFormat,
        loopEnabledValue);
}

AudioInput::~AudioInput() {
    if (sourceOwned)
        delete source;
    source = NULL;
}

const PcmFormat& AudioInput::format() const { return source->format(); }

int AudioInput::read(char* dst, int rawSize, int samplesRequested) {
    if (finished)
        return 0;

    int samplesRead = source ? source->read(dst, rawSize, samplesRequested) : 0;

    if ((samplesRead == 0) && source && source->canFinish()) {
        if (loopEnabledValue) {
            log.debug("audio input: source reached end; rewinding\n");
            source->rewind();
            samplesRead = source->read(dst, rawSize, samplesRequested);
            if (samplesRead == 0) {
                log.debug("audio input: source remained empty after rewind\n");
                finished = 1;
            }
        } else {
            log.debug("audio input: source reached end\n");
            finished = 1;
        }
    }

    if (samplesRead < 0)
        samplesRead = 0;

    return samplesRead;
}

int AudioInput::rawBufferSize(int frameRawSize, int samplesRequested) const {
    return source ? source->rawBufferSize(frameRawSize, samplesRequested) : frameRawSize;
}

int AudioInput::isFinished() const { return finished; }

void AudioInput::update() {
    if (source)
        source->update();
    finished = 0;
}

WavPcmSource::WavPcmSource(const char* name_, LogSink& log_)
    : PcmSource(log_)
    , file(NULL)
    , dataStart(0)
    , dataLength(0)
    , dataRead(0) {
    strncpy(name, name_, PATH_MAX - 1);
    name[PATH_MAX - 1] = '\0';
    if (open())
        error = 1;
}

WavPcmSource::~WavPcmSource() {
    if (file)
        pcmSourceClose(file);
}

int WavPcmSource::open() {
    if (file)
        pcmSourceClose(file);

    log.info("Playing file '%s'.\n", name);
    log.debug("wav pcm source: opening `%s'\n", name);

    file = fopen(name, "rb");
    if (file == NULL) {
        log.errorErrno(errno, "Can not open sound file `%s' for reading.", name);
        return 1;
    }

    return parseHeader();
}

int WavPcmSource::readChunkHeader(char id[4], unsigned int& size) {
    unsigned char sizeBytes[4];

    if (fread(id, 1, 4, file) != 4)
        return 1;
    if (fread(sizeBytes, 1, 4, file) != 4)
        return 1;

    size = readLe32(sizeBytes);
    return 0;
}

int WavPcmSource::parseHeader() {
    char id[4];
    unsigned int size;
    unsigned char formatBytes[16];
    int foundFormat = 0;
    int foundData = 0;

    dataStart = 0;
    dataLength = 0;
    dataRead = 0;

    if (readChunkHeader(id, size)) {
        log.error("Can not read WAV RIFF header.\n");
        return 1;
    }
    if ((memcmp(id, "RIFF", 4) != 0) && (memcmp(id, "RIFX", 4) != 0)) {
        log.warn("  Error in .wav header\n");
        return 1;
    }
    if (memcmp(id, "RIFX", 4) == 0) {
        log.warn("  Big-endian RIFX WAV files are not supported yet.\n");
        return 1;
    }
    if (fread(id, 1, 4, file) != 4) {
        log.error("Can not read WAV type.\n");
        return 1;
    }
    if (memcmp(id, "WAVE", 4) != 0) {
        log.warn("  Error in .wav header\n");
        return 1;
    }

    while (!foundData && !feof(file)) {
        if (readChunkHeader(id, size))
            break;

        if (memcmp(id, "fmt ", 4) == 0) {
            if (size < 16) {
                log.warn("  Unsupported .wav fmt chunk size %u.\n", size);
                return 1;
            }
            if (fread(formatBytes, 1, 16, file) != 16) {
                log.error("Can not read WAV format chunk.\n");
                return 1;
            }

            unsigned int audioFormat = readLe16(formatBytes);
            unsigned int channels = readLe16(formatBytes + 2);
            unsigned int sampleRate = readLe32(formatBytes + 4);
            unsigned int bitsPerSample = readLe16(formatBytes + 14);

            if (audioFormat != 1) {
                log.warn("  Unsupported .wav encoding %u.\n", audioFormat);
                return 1;
            }
            if ((channels < 1) || (channels > 2)) {
                log.warn("  Unsupported .wav channel count %u.\n", channels);
                return 1;
            }

            pcmFormat.channels = channels;
            pcmFormat.sampleRate = sampleRate;
            switch (bitsPerSample) {
            case 8:
                pcmFormat.sampleFormat = SF_u8;
                break;
            case 16:
                pcmFormat.sampleFormat = SF_s16_le;
                break;
            default:
                log.warn("  Unsupported .wav sample size %u.\n", bitsPerSample);
                return 1;
            }

            log.debug("wav pcm source: format rate=%d channels=%d sample-format=%d\n",
                pcmFormat.sampleRate, pcmFormat.channels, pcmFormat.sampleFormat);

            if (size > 16)
                fseek(file, size - 16, SEEK_CUR);
            if (size & 1)
                fseek(file, 1, SEEK_CUR);

            foundFormat = 1;
        } else if (memcmp(id, "data", 4) == 0) {
            if (!foundFormat) {
                log.warn("  WAV data chunk arrived before fmt chunk.\n");
                return 1;
            }
            dataStart = ftell(file);
            dataLength = size;
            dataRead = 0;
            foundData = 1;
            log.debug("wav pcm source: data-start=%ld data-length=%ld\n", dataStart, dataLength);
        } else {
            fseek(file, size, SEEK_CUR);
            if (size & 1)
                fseek(file, 1, SEEK_CUR);
        }
    }

    if (!foundData) {
        log.warn("  Error in .wav header\n");
        return 1;
    }

    return 0;
}

int WavPcmSource::read(char* dst, int rawSize, int samplesRequested) {
    if ((file == NULL) || error)
        return 0;

    if (dataRead >= dataLength)
        return 0;

    int bytesPerSample = pcmFormat.bytesPerSample();
    int maxSamples = (bytesPerSample > 0) ? rawSize / bytesPerSample : 0;
    int samples = min(samplesRequested, maxSamples);
    if ((bytesPerSample <= 0) || (samples <= 0))
        return 0;

    int remaining = dataLength - dataRead;
    int wanted = min(pcmBytesForSamples(samples, bytesPerSample), remaining);
    wanted -= wanted % bytesPerSample;
    if (wanted <= 0)
        return 0;
    int readBytes = fread(dst, 1, wanted, file);

    if (readBytes < 0)
        readBytes = 0;
    readBytes -= readBytes % bytesPerSample;
    dataRead += readBytes;

    return readBytes / bytesPerSample;
}

int WavPcmSource::canFinish() const { return 1; }

void WavPcmSource::rewind() {
    log.debug("wav pcm source: rewind `%s'\n", name);
    if ((file == NULL) || error)
        return;

    fseek(file, dataStart, SEEK_SET);
    dataRead = 0;
}

Minimp3PcmSource::Minimp3PcmSource(const char* name_, LogSink& log_)
    : PcmSource(log_)
    , decoder(NULL) {
    strncpy(name, name_, PATH_MAX - 1);
    name[PATH_MAX - 1] = '\0';
    if (open())
        error = 1;
}

Minimp3PcmSource::~Minimp3PcmSource() {
#if WITH_MINIMP3 == 1
    if (decoder) {
        mp3dec_ex_close((mp3dec_ex_t*)decoder);
        delete (mp3dec_ex_t*)decoder;
    }
#endif
    decoder = NULL;
}

int Minimp3PcmSource::open() {
#if WITH_MINIMP3 == 1
    log.info("Playing file '%s'.\n", name);
    log.debug("minimp3 pcm source: opening `%s'\n", name);

    mp3dec_ex_t* dec = new mp3dec_ex_t;
    memset(dec, 0, sizeof(*dec));

    int result = mp3dec_ex_open(dec, name, MP3D_SEEK_TO_SAMPLE | MP3D_DO_NOT_SCAN);
    if (result != 0) {
        log.warn("  Can not open MP3 file `%s' using minimp3: %d.\n", name, result);
        delete dec;
        return 1;
    }

    decoder = dec;
    return applyFormat();
#else
    log.warn("  Embedded minimp3 decoder support is not compiled in.\n");
    log.debug("minimp3 pcm source: open failed because WITH_MINIMP3=0 file=`%s'\n", name);
    return 1;
#endif
}

int Minimp3PcmSource::applyFormat() {
#if WITH_MINIMP3 == 1
    mp3dec_ex_t* dec = (mp3dec_ex_t*)decoder;
    if (dec == NULL)
        return 1;

    pcmFormat.sampleRate = dec->info.hz;
    pcmFormat.channels = dec->info.channels;
#ifdef WORDS_BIGENDIAN
    pcmFormat.sampleFormat = SF_s16_be;
#else
    pcmFormat.sampleFormat = SF_s16_le;
#endif

    if ((pcmFormat.channels < 1) || (pcmFormat.channels > 2) || (pcmFormat.sampleRate <= 0)) {
        log.warn("  Unsupported MP3 format rate=%d channels=%d.\n",
            pcmFormat.sampleRate, pcmFormat.channels);
        return 1;
    }

    log.debug("minimp3 pcm source: format rate=%d channels=%d sample-format=%d\n",
        pcmFormat.sampleRate, pcmFormat.channels, pcmFormat.sampleFormat);
    return 0;
#else
    return 1;
#endif
}

int Minimp3PcmSource::read(char* dst, int rawSize, int samplesRequested) {
#if WITH_MINIMP3 == 1
    if ((decoder == NULL) || error)
        return 0;

    int bytesPerSample = pcmFormat.bytesPerSample();
    int maxSamples = (bytesPerSample > 0) ? rawSize / bytesPerSample : 0;
    int samples = min(samplesRequested, maxSamples);
    if (samples <= 0)
        return 0;

    int channels = pcmFormat.channels;
    if (channels <= 0)
        return 0;

    size_t sampleValuesWanted = (size_t)samples * (size_t)channels;
    size_t sampleValuesRead = mp3dec_ex_read((mp3dec_ex_t*)decoder, (mp3d_sample_t*)dst,
        sampleValuesWanted);

    return (int)(sampleValuesRead / channels);
#else
    return 0;
#endif
}

int Minimp3PcmSource::canFinish() const { return 1; }

void Minimp3PcmSource::rewind() {
    log.debug("minimp3 pcm source: rewind `%s'\n", name);
#if WITH_MINIMP3 == 1
    if ((decoder == NULL) || error)
        return;

    mp3dec_ex_seek((mp3dec_ex_t*)decoder, 0);
#endif
}

RawPcmSource::RawPcmSource(const char* name_, const PcmFormat& format,
    LogSink& log_)
    : PcmSource(log_)
    , file(NULL) {
    pcmFormat = format;
    strncpy(name, name_ ? name_ : "", PATH_MAX - 1);
    name[PATH_MAX - 1] = '\0';
    if (open())
        error = 1;
}

RawPcmSource::~RawPcmSource() {
    if (file)
        pcmSourceClose(file);
    file = NULL;
}

int RawPcmSource::open() {
    if (file)
        pcmSourceClose(file);

    log.info("Playing raw PCM file '%s'.\n", name);
    log.debug("raw pcm source: opening `%s'\n", name);

    file = fopen(name, "rb");
    if (file == NULL) {
        log.errorErrno(errno, "Can not open sound file `%s' for reading.", name);
        return 1;
    }

    return applyFormat();
}

int RawPcmSource::applyFormat() {
    if (pcmFormat.sampleRate <= 0) {
        log.warn("  Unsupported raw audio sample rate %d.\n", pcmFormat.sampleRate);
        return 1;
    }
    if ((pcmFormat.channels < 1) || (pcmFormat.channels > 2)) {
        log.warn("  Unsupported raw audio channel count %d.\n", pcmFormat.channels);
        return 1;
    }
    if ((pcmFormat.sampleFormat < SF_u8) || (pcmFormat.sampleFormat > SF_s16_be)) {
        log.warn("  Unsupported raw audio format %d.\n", pcmFormat.sampleFormat);
        return 1;
    }
    if (pcmFormat.bytesPerSample() <= 0) {
        log.warn("  Unsupported raw audio format %d.\n", pcmFormat.sampleFormat);
        return 1;
    }

    log.debug("raw pcm source: format rate=%d channels=%d sample-format=%d\n",
        pcmFormat.sampleRate, pcmFormat.channels, pcmFormat.sampleFormat);
    return 0;
}

int RawPcmSource::read(char* dst, int rawSize, int samplesRequested) {
    if ((file == NULL) || error)
        return 0;

    int bytesPerSample = pcmFormat.bytesPerSample();
    int maxSamples = (bytesPerSample > 0) ? rawSize / bytesPerSample : 0;
    int samples = min(samplesRequested, maxSamples);
    if (samples <= 0)
        return 0;

    int wanted = pcmBytesForSamples(samples, bytesPerSample);
    int readBytes = fread(dst, 1, wanted, file);
    readBytes -= readBytes % bytesPerSample;

    return readBytes / bytesPerSample;
}

int RawPcmSource::canFinish() const { return 1; }

void RawPcmSource::rewind() {
    log.debug("raw pcm source: rewind `%s'\n", name);
    if ((file == NULL) || error)
        return;

    clearerr(file);
    if (fseek(file, 0, SEEK_SET) != 0)
        log.debug("raw pcm source: rewind failed for `%s' errno=%d\n", name, errno);
}

RandomNoisePcmSource::RandomNoisePcmSource(const PcmFormat& requestedFormat,
    RandomSource& randomSource_, LogSink& log_)
    : PcmSource(log_)
    , randomSource(randomSource_)
    , v1(0)
    , v2(0)
    , maxdv(2) {
    pcmFormat.sampleRate = requestedFormat.sampleRate;
    if (pcmFormat.sampleRate <= 0)
        pcmFormat.sampleRate = 44100;
    pcmFormat.channels = 2;
    pcmFormat.sampleFormat = SF_u8;
    log.debug("random noise pcm source: created rate=%d channels=%d format=%d\n",
        pcmFormat.sampleRate, pcmFormat.channels, pcmFormat.sampleFormat);
}

int RandomNoisePcmSource::read(char* dst, int rawSize, int samplesRequested) {
    int bytesPerSample = pcmFormat.bytesPerSample();
    int maxSamples = (bytesPerSample > 0) ? rawSize / bytesPerSample : 0;
    int frames = min(min(samplesRequested, maxSamples), 256);

    if (frames <= 0)
        return 0;

    unsigned char* soundData = (unsigned char*)dst;

    soundData[0] = 144;
    soundData[1] = 112;
    for (int x = 1; x < frames; x++) {
        unsigned char* current = soundData + x * 2;
        unsigned char* previous = current - 2;

        if (randomSource.uniformInt(256) > previous[0])
            v1 += randomSource.uniformInt(maxdv);
        else
            v1 -= randomSource.uniformInt(maxdv);

        if (randomSource.uniformInt(256) > previous[1])
            v2 += randomSource.uniformInt(maxdv);
        else
            v2 -= randomSource.uniformInt(maxdv);

        current[0] = previous[0] + v1;
        current[1] = previous[1] + v2;
    }

    return frames;
}

void RandomNoisePcmSource::rewind() {
    v1 = 0;
    v2 = 0;
}
