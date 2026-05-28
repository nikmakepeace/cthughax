#include "cthugha.h"
#include "Sound.h"
#include "imath.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>

#if WITH_PULSE == 1
#include <pulse/error.h>
#include <pulse/simple.h>
#endif

#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

char SoundDeviceFile::name[PATH_MAX] = "";
char SoundDeviceFile::fifo[PATH_MAX] = "/tmp/cthugha.com";
char SoundDeviceFile::fifoDir[PATH_MAX] = "";

int soundPlayLoop = 1; // play file(s) over and over again

#define RIFF 0x46464952
#define WAVE 0x45564157
#define FMT 0x20746D66
#define DATA 0x61746164
#define PCM_CODE 1

#if WITH_PULSE == 1
class SoundOutputPulse : public SoundOutputDevice {
    pa_simple* pulse;
    pa_sample_spec sampleSpec;

    int bytesPerSecond() const;

public:
    SoundOutputPulse();
    virtual ~SoundOutputPulse();

    int write(const void* buffer, int size);
    int outputDelayBytes() const;
    int isOpen() const { return pulse != NULL; }
    void update();
};
#endif

typedef struct _waveheader {
    char main_chunk[4]; /* 'RIFF' */
    uint32_t length; /* filelen */
    char chunk_type[4]; /* 'WAVE' */

    char sub_chunk[4]; /* 'fmt ' */
    uint32_t sc_len; /* length of sub_chunk, =16 */
    uint16_t format; /* should be 1 for PCM-code */
    uint16_t modus; /* 1 Mono, 2 Stereo */
    uint32_t sample_fq; /* frequence of sample */
    uint32_t byte_p_sec;
    uint16_t byte_p_spl; /* samplesize; 1 or 2 bytes */
    uint16_t bit_p_spl; /* 8, 12 or 16 bit */

    char data_chunk[4]; /* 'data' */
    uint32_t data_length; /* samplecount */
} WaveHeader;

typedef char WaveHeader_must_match_pcm_wav_header_size[(sizeof(WaveHeader) == 44) ? 1 : -1];

static uint16_t read_le16(uint16_t value) {
    unsigned char* p = (unsigned char*)&value;
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t read_le32(uint32_t value) {
    unsigned char* p = (unsigned char*)&value;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

#if WITH_PULSE == 1
static pa_sample_format_t pulse_sample_format() {
    switch (soundFormat) {
    case SF_u8:
        return PA_SAMPLE_U8;
    case SF_s16_le:
        return PA_SAMPLE_S16LE;
    case SF_s16_be:
        return PA_SAMPLE_S16BE;
    default:
        return PA_SAMPLE_INVALID;
    }
}

int SoundOutputPulse::bytesPerSecond() const {
    return pa_bytes_per_second(&sampleSpec);
}

SoundOutputPulse::SoundOutputPulse()
    : pulse(NULL) {
    update();
}

SoundOutputPulse::~SoundOutputPulse() {
    if (pulse)
        pa_simple_free(pulse);
    pulse = NULL;
}

void SoundOutputPulse::update() {
    int error;

    if (pulse) {
        pa_simple_free(pulse);
        pulse = NULL;
    }

    sampleSpec.format = pulse_sample_format();
    sampleSpec.rate = int(soundSampleRate);
    sampleSpec.channels = int(soundChannels);

    if (sampleSpec.format == PA_SAMPLE_INVALID) {
        CTH_DEBUG("    sound output strategy: Pulse passthrough unavailable for format `%s'\n",
            soundFormat.text());
        return;
    }

    CTH_DEBUG("    sound output strategy: opening Pulse server `%s'\n",
        pulse_server_display_name());
    pulse = pa_simple_new(pulse_server_name(), "Cthughanix", PA_STREAM_PLAYBACK, NULL,
        "Audio passthrough", &sampleSpec, NULL, NULL, &error);
    if (pulse == NULL) {
        CTH_DEBUG("    sound output strategy: Pulse server `%s' failed to open: %s\n",
            pulse_server_display_name(), pa_strerror(error));
    } else {
        CTH_DEBUG("    sound output strategy: Pulse server `%s' opened successfully\n",
            pulse_server_display_name());
        CTH_TRACE("opened server=`%s' rate=%d channels=%d format=%d\n",
            "sound pulse output", pulse_server_display_name(),
            sampleSpec.rate, sampleSpec.channels, sampleSpec.format);
    }
}

int SoundOutputPulse::write(const void* buffer, int size) {
    int error;

    if (pulse == NULL)
        return 0;

    if (pa_simple_write(pulse, buffer, size, &error) < 0) {
        CTH_ERROR("Pulse passthrough write failed on server `%s': %s\n",
            pulse_server_display_name(), pa_strerror(error));
        return 0;
    }

    return size;
}

int SoundOutputPulse::outputDelayBytes() const {
    int error;
    pa_usec_t latency;

    if (pulse == NULL)
        return 0;

    latency = pa_simple_get_latency(pulse, &error);
    if (latency == (pa_usec_t)-1)
        return 0;

    return (int)((latency * bytesPerSecond()) / 1000000);
}
#endif

int SoundDeviceFile::hasSoundOutputDevice() {
#if WITH_PULSE == 1
    CTH_DEBUG("    sound output strategy: Pulse passthrough is compiled in\n");
    return 1;
#endif
#if WITH_DSP == 1
    if (SoundDeviceDSP::dev_dsp[0] == '\0') {
        CTH_DEBUG("    sound output strategy: none, because no OSS DSP device is configured\n");
        return 0;
    }
    if (access(SoundDeviceDSP::dev_dsp, W_OK) != 0) {
        CTH_DEBUG("    sound output strategy: none, because `%s' is not writable\n",
            SoundDeviceDSP::dev_dsp);
        return 0;
    }
    CTH_DEBUG("    sound output strategy: OSS DSP passthrough, because `%s' is writable\n",
        SoundDeviceDSP::dev_dsp);
    return 1;
#else
    CTH_DEBUG("    sound output strategy: none, because OSS DSP support is not compiled in\n");
    return 0;
#endif
}

SoundOutputDevice* SoundDeviceFile::newOutputDevice() {
    if (soundSilent) {
        CTH_DEBUG("    sound output strategy: none, because audio passthrough is disabled by silent playback\n");
        return NULL;
    }

#if WITH_PULSE == 1
    SoundOutputPulse* pulse = ::new SoundOutputPulse;
    if (pulse->isOpen()) {
        CTH_DEBUG("    sound output strategy: Pulse passthrough opened successfully\n");
        return pulse;
    }
    delete pulse;
    CTH_DEBUG("    sound output strategy: trying OSS DSP passthrough after Pulse failed\n");
#endif

#if WITH_DSP != 1
    CTH_DEBUG("    sound output strategy: none, because OSS DSP support is not compiled in\n");
    return NULL;
#else
    if (!hasSoundOutputDevice())
        return NULL;

    SoundDeviceDSPOut* device = ::new SoundDeviceDSPOut;
    if (device->getHandle() < 0) {
        CTH_DEBUG("    sound output strategy: none, because OSS DSP passthrough failed to open\n");
        delete device;
        return NULL;
    }

    CTH_DEBUG("    sound output strategy: OSS DSP passthrough opened successfully\n");
    return device;
#endif
}

SoundDeviceFile::SoundDeviceFile()
    : SoundDevice()
    , file(NULL)
    , output(NULL)
    , bufferPid(-1)
    , childPid(-1)
    , playbackHistory(NULL)
    , playbackHistorySize(0)
    , playbackHistoryWritten(0) {

    strncpy(fifoDir, "/tmp/cthugha.XXXXXX", PATH_MAX);
    fifoDir[PATH_MAX - 1] = '\0';
    if (mkdtemp(fifoDir) == NULL) {
        CTH_ERRNO(errno, "Can not create temporary directory.");
        error = 1;
    } else {
        int n = snprintf(fifo, PATH_MAX, "%s/sound", fifoDir);
        if ((n < 0) || (n >= PATH_MAX)) {
            CTH_ERROR("Temporary sound FIFO path too long.\n");
            error = 1;
        }
    }

    output = newOutputDevice();

    //
    // set up memory for sound buffer
    //
    bufferChunkSize = 4096;
    bufferSize = int(soundBuffer) * 1024;
    bufferPos = 0; // start of data in the buffer (read position)
    bufferFill = 0; // number of elements in the buffer

    if (bufferSize <= (4 * bufferChunkSize)) {
        buffer = NULL;
        bufferSize = 0;
    } else {
        buffer = new unsigned char[bufferSize + bufferChunkSize];
    }
}

void SoundDeviceFile::fillRawSilence(unsigned char* dst, int n) {
    switch (soundFormat) {
    case SF_u8:
        memset(dst, 128, n);
        break;
    case SF_u16_le:
        for (int i = 0; i + 1 < n; i += 2) {
            dst[i] = 0;
            dst[i + 1] = 128;
        }
        break;
    case SF_u16_be:
        for (int i = 0; i + 1 < n; i += 2) {
            dst[i] = 128;
            dst[i + 1] = 0;
        }
        break;
    default:
        memset(dst, 0, n);
    }
}

void SoundDeviceFile::rememberPlayback(const unsigned char* data, int n) {
    int written = 0;

    if (n <= 0)
        return;

    if (playbackHistory == NULL) {
        playbackHistorySize = max(int(soundBuffer) * 1024, rawSize * 64);
        playbackHistorySize = max(playbackHistorySize, 256 * 1024);
        playbackHistory = new unsigned char[playbackHistorySize];
        fillRawSilence(playbackHistory, playbackHistorySize);
        playbackHistoryWritten = 0;
    }

    while (written < n) {
        int pos = playbackHistoryWritten % playbackHistorySize;
        int chunk = min(n - written, playbackHistorySize - pos);

        memcpy(playbackHistory + pos, data + written, chunk);
        playbackHistoryWritten += chunk;
        written += chunk;
    }
}

void SoundDeviceFile::copyPlaybackHistory(long long pos, unsigned char* dst, int n) {
    int copied = 0;

    while (copied < n) {
        int historyPos = pos % playbackHistorySize;
        int chunk = min(n - copied, playbackHistorySize - historyPos);

        memcpy(dst + copied, playbackHistory + historyPos, chunk);
        pos += chunk;
        copied += chunk;
    }
}

int SoundDeviceFile::copyPlaybackAtOutputTime(int n) {
    int delay;
    long long played;
    long long start;
    int silence;

    if ((output == NULL) || (playbackHistory == NULL) || (n <= 0))
        return 0;

    delay = output->outputDelayBytes();
    if (delay <= 0)
        return 0;

    delay -= delay % bytesPerSample;
    n -= n % bytesPerSample;

    played = playbackHistoryWritten - delay;
    start = played - n;

    if (played <= 0) {
        fillRawSilence((unsigned char*)tmpData, n);
        return n;
    }

    if (start < playbackHistoryWritten - playbackHistorySize)
        return 0;

    if (start < 0) {
        silence = min(n, int(-start));
        fillRawSilence((unsigned char*)tmpData, silence);
        copyPlaybackHistory(0, (unsigned char*)tmpData + silence, n - silence);
    } else {
        copyPlaybackHistory(start, (unsigned char*)tmpData, n);
    }

    return n;
}

//
// play the next file (currently just one file over and over again)
//
int SoundDeviceFile::playNext() {

    static int rep = 0;

    if ((rep > 0) && !soundPlayLoop) { // stop playing
        CTH_INFO("Stopping...\n");
        if (soundSilent) {
            exit(0);
        } else {
            kill(getppid(), 15); // kill parent (SoundDeviceFork process)
            abort();
        }
    }

    rep++;
    CTH_INFO("Playing file '%s'.\n", name);

    if (open()) { // open file / start program
        error = 1;
        return 1;
    }

    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    rawSize = bytesPerSample * size;
    if (tmpDataOwned)
        setTmpData();

    if (output)
        output->update();
    sound_communicate(0);

    return 0;
}

//
// open the sound file, or
// start the sound reading program
//
int SoundDeviceFile::open() {
#if (MOD_PLAYER == 1) || (MP3_PLAYER == 1) || (MP3_PLAYER == 2)
    char prog[PATH_MAX * 6];
#endif
    //
    // convert file name to lowercase
    //
    char name_lc[PATH_MAX + 1];
    int i;
    for (i = 0; (i < PATH_MAX) && (name[i] != '\0'); i++)
        name_lc[i] = tolower(name[i]);
    name_lc[i] = '\0';

    if (strstr(name_lc, ".wav") != NULL) {
        //
        // play a .wav file
        //
        CTH_DEBUG("    Playing .wav file.\n");

        if (openFile())
            return 1;
        if (wavHeader())
            return 1;

        return 0;

    } else if (strstr(name_lc, ".mod") != NULL) {
        //
        // play a mod file
        //

#if MOD_PLAYER == 1
        //
        // play using the 'xmp' player
        //
        CTH_DEBUG("    Playing MOD file using 'xmp'.\n");

        soundFormat.setValue(SF_s16_le);
        soundChannels.setValue(2);
        soundSampleRate.setValue(22050);

        sprintf(prog, "%s -c -q --stereo -b 16 -f 22050 --little-endian  \"%s\" > %s ", MOD_PATH,
            name, fifo);

        if (openProg(prog))
            return 1;

#else

        CTH_ERROR("Sorry. No MOD Player configured.\n");
        return 1;

#endif

        return 0;

    } else if (strstr(name_lc, ".mp3") != NULL) {
        //
        // play a MPEG Layer3 file
        //

#if MP3_PLAYER == 1
        //
        // start mpg123. decode first frame to get sample rate, ...
        //
        // according to mpg123 man page the output is raw linear PCM,
        // signed, 16 bit, host byte order
        //
        // I get the frequency from the output of mpg123 (by deconding
        // just a little bit). This works as long as the output
        // does not change.
        // current version of mpg123: 0.59f
        //
        // It would be nice if mpg123 would write a .wav header.
        //
        CTH_DEBUG("    Playing MP3 file using 'mpg123'.\n");

        CTH_DEBUG("    decoding first frame to get sound header...\n");

        systemf("%s -s -n 1 \"%s\" 2> %s > /dev/null", MP3_PATH, name, fifo);

        if ((file = fopen(fifo, "r")) == NULL) {
            CTH_ERRNO(errno, "Can not open the temporary file '%s'.\n", fifo);
            return 1;
        }

#if (__BYTE_ORDER == __BIG_ENDIAN)
        soundFormat.setValue(SF_s16_be);
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
        soundFormat.setValue(SF_s16_le);
#endif
        while (!feof(file)) {
            char line[512];
            fgets(line, 512, file);

            char* p;
            if ((p = strstr(line, "Freq:")) != NULL) {
                int freq;
                sscanf(p, "Freq:%d", &freq);
                soundSampleRate.setValue(freq);
            }
            if ((p = strstr(line, "Channels:")) != NULL) {
                int chnls;
                sscanf(p, "Channels:%d", &chnls);
                soundChannels.setValue(chnls);
            }
        }
        fclose0(file);
        unlink(fifo);

        CTH_DEBUG("Starting 'mpg123' for decoding...\n");
        sprintf(prog, "%s -s \"%s\" > %s", MP3_PATH, name, fifo);
        if (openProg(prog))
            return 1;

#elif MP3_PLAYER == 2

        //
        // play MP3 file using the 'l3dec' player
        //
        // method simmilar to mpg123, but use the .wav header
        //
        CTH_DEBUG("    Playing MP3 file using 'l3dec'.\n");

        CTH_DEBUG("    decoding first frame to get sound header...\n");

        systemf("echo \"n\" | %s -wav -fn 0 \"%s\" %s 2> /dev/null", MP3_PATH, name, fifo);
        if ((file = fopen(fifo, "r")) == NULL) {
            CTH_ERROR("Can not open temporary file.\n");
            return 1;
        }
        if (wavHeader()) {
            fclose(file);
            return 1;
        }
        fclose0(file);
        unlink(fifo);

        sprintf(prog, "echo \"n\" | %s -wav \"%s\" %s", MP3_PATH, name, fifo);
        if (openProg(prog))
            return 1;

        if (wavHeader()) {
            fclose(file);
            return 1;
        }

#else
        CTH_ERROR("Sorry. No MP3 Player configured.\n");
        return 1;
#endif

    } else {
        CTH_DEBUG("    Playing raw sound data..\n");

        if (openFile())
            return 1;
    }

    return 0;
}

int SoundDeviceFile::openFile() {
    if ((file = fopen(name, "rb")) == NULL) {
        CTH_ERRNO(errno, "Can not open sound file `%s' for reading.", name);
        return 1;
    }

    return 0;
}

int SoundDeviceFile::openProg(char* prog) {

    mkfifo(fifo, 0666);

    switch (childPid = fork()) {
    case -1:
        CTH_ERRNO(errno, "Can not fork for sound reading program.\n");
        error = 1;
        return 1;
    case 0: {
        if (output)
            delete output;

        CTH_DEBUG("    starting sound reader `%s'.\n", prog);

        char sh[] = "sh";
        char flag[] = "-c";
        char* argv[4];
        argv[0] = sh;
        argv[1] = flag;
        argv[2] = prog;
        argv[3] = 0;
        execv("/bin/sh", argv); // should not return

        CTH_ERRNO(errno, "execve failed.");

        abort();
        break;
    }
    default:
        // now open this pipe for reading
        CTH_DEBUG("    opening '%s'\n", fifo);
        if ((file = fopen(fifo, "r")) == NULL) {
            CTH_ERRNO(errno, "Can not open communication pipe `%s'.\n", fifo);
            return 1;
        }
    }
    return 0;
}

int SoundDeviceFile::wavHeader() {
    WaveHeader header;

    if (fread(&header, sizeof(header), 1, file) != 1) {
        if (feof(file)) {
            CTH_ERROR("Can not read header (end of file).\n");
        } else {
            CTH_ERRNO(errno, "Can not read header.");
        }
        return 1;
    }

    /* is it a .wav file */
    if ((memcmp(header.main_chunk, "RIFF", 4) == 0) && (memcmp(header.chunk_type, "WAVE", 4) == 0)
        && (memcmp(header.sub_chunk, "fmt ", 4) == 0) && (memcmp(header.data_chunk, "data", 4) == 0)
        && (read_le16(header.format) == PCM_CODE)) {

        soundChannels.setValue(read_le16(header.modus));
        switch (read_le16(header.bit_p_spl)) {
        case 8:
            soundFormat.setValue(SF_u8);
            break;
        case 16:
            soundFormat.setValue(SF_s16_le);
            break;
        default:
            CTH_WARN("  Unsupported .wav sample size %d.\n", read_le16(header.bit_p_spl));
            return 1;
        }
        soundSampleRate.setValue(read_le32(header.sample_fq));
    } else {
        CTH_WARN("  Error in .wav header\n");
        return 1;
    }
    return 0;
}

int SoundDeviceFile::read() {
    int r = 0, w = 0;
    int visualBytes = 0;

    //
    // an error occured earlier, do nothing
    //
    if (error)
        return 1;

    //
    // start playing a new file
    //
    if ((file == NULL)) {
        if (playNext())
            return 1;
    }

    //
    // check for end of file
    //
    if (feof(file) || ferror(file)) {
        if (bufferFill > 0) {
            //
            // write remaingin data from buffer
            //
            w = min(bufferFill, rawSize);

            unsigned char* writePos = buffer + bufferPos;

            w = output ? output->write(writePos, w) : w; // to soundcard
            if (output && (w > 0)) {
                rememberPlayback(writePos, w);
                visualBytes = copyPlaybackAtOutputTime(rawSize);
            }
            if (visualBytes <= 0) {
                memcpy(tmpData, writePos, w); // to shared memory
                visualBytes = w;
            }

            bufferPos = (bufferPos + w) % bufferSize;
            bufferFill = bufferFill - w;
        } else {
            //
            // close the file
            //
            close();
        }
    } else if (output == NULL) {
        //
        // playing silently -> no buffering needed
        //
        w = fread(tmpData, 1, rawSize, file);
        visualBytes = w;

    } else if (bufferSize == 0) {
        //
        // play without bufferering
        //
        w = fread(tmpData, 1, rawSize, file);
        w = output->write(tmpData, w);
        if (w > 0) {
            rememberPlayback((unsigned char*)tmpData, w);
            visualBytes = copyPlaybackAtOutputTime(rawSize);
        }
        if (visualBytes <= 0)
            visualBytes = w;

    } else {
        //
        // read the sound, do buffering, and write to soundcard
        //
        // The file buffer decouples decoder/file reads from the visual frame
        // loop, but playback still writes rawSize bytes at a time. rawSize is
        // one visual-analysis slice, so this is not necessarily the sound
        // backend's preferred output chunk size.
        unsigned char* readPos = buffer + (bufferPos + bufferFill) % bufferSize;
        unsigned char* writePos = buffer + bufferPos;

        if (bufferFill < rawSize) { // buffer empty

            r = fread(readPos, 1, bufferChunkSize, file);

        } else if ((bufferFill + bufferChunkSize) >= bufferSize) { // buffer full

            w = output->write(writePos, rawSize); // to soundcard, in visual-slice-sized chunks
            if (w > 0) {
                rememberPlayback(writePos, w);
                visualBytes = copyPlaybackAtOutputTime(rawSize);
            }
            if (visualBytes <= 0) {
                memcpy(tmpData, writePos, w); // to shared memory for visual analysis
                visualBytes = w;
            }

        } else {
            int fileHandle = fileno(file);
            int outputHandle = output->getHandle();

            if (fileHandle < 0) {
                CTH_ERROR("Invalid sound file handle.\n");
                error = 1;
                return 1;
            }

            if (outputHandle < 0) {
                r = fread(readPos, 1, bufferChunkSize, file);
                w = output->write(writePos, rawSize); // to soundcard, in visual-slice-sized chunks
                if (w > 0) {
                    rememberPlayback(writePos, w);
                    visualBytes = copyPlaybackAtOutputTime(rawSize);
                }
                if (visualBytes <= 0) {
                    memcpy(tmpData, writePos, w); // to shared memory for visual analysis
                    visualBytes = w;
                }
            } else {
                fd_set rfds, wfds;

                FD_ZERO(&rfds);
                FD_ZERO(&wfds);

                FD_SET(fileHandle, &rfds);
                FD_SET(outputHandle, &wfds);

                switch (select(FD_SETSIZE, &rfds, &wfds, NULL, NULL)) {
                case -1:
                    CTH_ERRNO(errno, "Error in select.");
                    break;
                case 0:
                    break;
                default:
                    if (FD_ISSET(fileHandle, &rfds)) // reading possible
                        r = fread(readPos, 1, bufferChunkSize, file);
                    if (FD_ISSET(outputHandle, &wfds)) { // write possible
                        w = output->write(writePos, rawSize); // to soundcard, in visual-slice-sized chunks
                        if (w > 0) {
                            rememberPlayback(writePos, w);
                            visualBytes = copyPlaybackAtOutputTime(rawSize);
                        }
                        if (visualBytes <= 0) {
                            memcpy(tmpData, writePos, w); // to shared memory for visual analysis
                            visualBytes = w;
                        }
                    }
                }
            }
        }

        // check for read over the end of the buffer
        if ((r > 0) && (((bufferPos + bufferFill) % bufferSize + r) > bufferSize)) {
            memcpy(buffer, buffer + bufferSize,
                (bufferPos + bufferFill) % bufferSize + r - bufferSize);
        }

        // update buffer position and size
        bufferPos = (bufferPos + w) % bufferSize;
        bufferFill = bufferFill + r - w;
    }

    return visualBytes / bytesPerSample;
}

void SoundDeviceFile::update() {
    if (output)
        output->update();
}

int SoundDeviceFile::close() {

    if (childPid >= 0) {
        CTH_DEBUG("    waiting for sound reader to stop.\n");

        if (waitpid(childPid, NULL, WNOHANG) == 0) {
            CTH_DEBUG("    stopping sound reader\n");

            kill(childPid, SIGTERM);
            usleep(100000);
            if (waitpid(childPid, NULL, WNOHANG) == 0) {
                kill(childPid, SIGKILL);
                waitpid(childPid, NULL, 0);
            }
        }

        unlink(fifo);

        childPid = -1;
    }

    if (file)
        fclose0(file);

    return 0;
}

SoundDeviceFile::~SoundDeviceFile() {
    delete output;
    output = NULL;
    delete[] buffer;
    buffer = NULL;
    delete[] playbackHistory;
    playbackHistory = NULL;

    close();
    if (fifo[0] != '\0')
        unlink(fifo);
    if (fifoDir[0] != '\0')
        rmdir(fifoDir);
}
