#include "cthugha.h"
#include "Sound.h"
#include "imath.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>

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

typedef struct _waveheader {
    unsigned long main_chunk; /* 'RIFF' */
    unsigned long length; /* filelen */
    unsigned long chunk_type; /* 'WAVE' */

    unsigned long sub_chunk; /* 'fmt ' */
    unsigned long sc_len; /* length of sub_chunk, =16 */
    unsigned short format; /* should be 1 for PCM-code */
    unsigned short modus; /* 1 Mono, 2 Stereo */
    unsigned long sample_fq; /* frequence of sample */
    unsigned long byte_p_sec;
    unsigned short byte_p_spl; /* samplesize; 1 or 2 bytes */
    unsigned short bit_p_spl; /* 8, 12 or 16 bit */

    unsigned long data_chunk; /* 'data' */
    unsigned long data_length; /* samplecount */
} WaveHeader;

SoundDeviceFile::SoundDeviceFile()
    : SoundDevice()
    , file(NULL)
    , dsp(NULL)
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
        snprintf(fifo, PATH_MAX, "%s/sound", fifoDir);
    }

    if (!soundSilent)
        dsp = ::new SoundDeviceDSPOut;

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

    if ((dsp == NULL) || (playbackHistory == NULL) || (n <= 0))
        return 0;

    delay = dsp->outputDelayBytes();
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

    if (dsp)
        dsp->update();
    sound_communicate(0);

    return 0;
}

//
// open the sound file, or
// start the sound reading program
//
int SoundDeviceFile::open() {
    char prog[PATH_MAX * 6];
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
    if ((file = fopen(name, "r")) == NULL) {
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
    case 0:
        if (dsp)
            delete dsp;

        CTH_DEBUG("    starting sound reader `%s'.\n", prog);

        char* argv[4];
        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = prog;
        argv[3] = 0;
        execv("/bin/sh", argv); // should not return

        CTH_ERRNO(errno, "execve failed.");

        abort();
        break;
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

    if (fread(&header, sizeof(header), 1, file) <= 0) {
        if (feof(file))
            CTH_ERROR("Can not read header (end of file).\n");
        else
            CTH_ERRNO(errno, "Can not read header.");
    }

    /* is it a .wav file */
    if ((header.main_chunk == RIFF) && (header.chunk_type == WAVE)) {

        soundChannels.setValue(header.modus);
        switch (header.bit_p_spl) {
        case 8:
            soundFormat.setValue(SF_u8);
            break;
        case 16:
            soundFormat.setValue(SF_s16_le);
        }
        soundSampleRate.setValue(header.sample_fq);
    } else {
        CTH_WARN("  Error in .wav header\n");
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

            w = dsp ? dsp->write(writePos, w) : w; // to soundcard
            if (dsp && (w > 0)) {
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
    } else if (dsp == NULL) {
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
        w = dsp->write(tmpData, w);
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

            w = dsp->write(writePos, rawSize); // to soundcard, in visual-slice-sized chunks
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

            FD_SET(fileno(file), &rfds);
            FD_SET(dsp->getHandle(), &wfds);

            switch (select(FD_SETSIZE, &rfds, &wfds, NULL, NULL)) {
            case -1:
                CTH_ERRNO(errno, "Error in select.");
                break;
            case 0:
                break;
            default:
                if (FD_ISSET(fileno(file), &rfds)) // reading possible
                    r = fread(readPos, 1, bufferChunkSize, file);
                if (FD_ISSET(dsp->getHandle(), &wfds)) { // write possible
                    w = dsp->write(writePos, rawSize); // to soundcard, in visual-slice-sized chunks
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
    if (dsp)
        dsp->update();
}

int SoundDeviceFile::close() {

    if (file)
        fclose0(file);

    if (childPid >= 0) {
        CTH_DEBUG("    waiting for sound reader to stop.\n");

        if (waitpid(childPid, NULL, WNOHANG) == -1) {
            sleep(1);
            if (waitpid(childPid, NULL, WNOHANG) == -1) {
                CTH_DEBUG("    stopping sound reader\n");

                kill(SIGKILL, childPid);
                waitpid(childPid, NULL, 0);
            }
        }

        unlink(fifo);

        childPid = -1;
    }

    return 0;
}

SoundDeviceFile::~SoundDeviceFile() {
    delete dsp;
    dsp = NULL;
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
