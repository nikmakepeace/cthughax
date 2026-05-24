#include "cthugha.h"
#include "Sound.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <memory.h>

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

class sound_Communicator {
public:
    int sample_rate;
    int channels;
    int format;
    int method;
    int fragments;
    int fragment_size;

    int update;
    int done_update;
    int stop;
};
sound_Communicator* sound_communicator = NULL;

void sig_tty_parent(int) {
    CTH_INFO("Stopping in child...\n");

    kill(getppid(), SIGTSTP);

    sound_communicator->stop = 1;
}

int sound_comm_read();
int sound_communicate(int to_child) {
    if (sound_communicator == NULL)
        return 0;

    sound_communicator->sample_rate = int(soundSampleRate);
    sound_communicator->channels = int(soundChannels);
    sound_communicator->format = int(soundFormat);
    sound_communicator->method = int(soundDSPMethod);
    sound_communicator->fragments = int(soundDSPFragments);
    sound_communicator->fragment_size = int(soundDSPFragmentSize);
    if (to_child) {
        sound_communicator->update = 1;
        sound_communicator->done_update = 0;
    } else {
        sound_communicator->update = 0;
        sound_communicator->done_update = 1;
    }
    return 0;
}
int sound_comm_read() {
    if (sound_communicator == NULL)
        return 0;

    soundSampleRate.setValue(sound_communicator->sample_rate);
    soundChannels.setValue(sound_communicator->channels);
    soundFormat.setValue(sound_communicator->format);
    soundDSPMethod.setValue(sound_communicator->method);
    soundDSPFragments.setValue(sound_communicator->fragments);
    soundDSPFragmentSize.setValue(sound_communicator->fragment_size);

    return 0;
}

/*
 * fork the sound reading process
 */
SoundDeviceFork::SoundDeviceFork() {
    int sharedRawSize = 4 * size;

    CTH_DEBUG("    starting sound reading process...\n");
    if ((sound_shm_key
            = shmget(IPC_PRIVATE, sharedRawSize + sizeof(sound_Communicator), IPC_CREAT | 0777))
        == -1) {
        CTH_ERRNO(errno, "Can not create shared memory segment.");
        error = 1;
        return;
    }
    if ((sound_shared = shmat(sound_shm_key, 0, 0)) == (void*)-1) {
        CTH_ERRNO(errno, "Can not attach shared memory segment.");
        error = 1;
        return;
    }

    sound_communicator = (sound_Communicator*)sound_shared;
    sound_communicate(1);
    sound_communicator->stop = 0;

    // a little bit complicated to avoid waring
    sound_shared = (char*)(sound_shared) + sizeof(sound_Communicator);

    switch (sound_child = fork()) {
    case -1: /* error */
        CTH_ERRNO(errno, "Can not fork sound child.");
        shmdt((char*)sound_shared);

        error = 1;
        return;

    case 0: /* child */
        audioRuntimeInit(RSIC_FileChild, 0);

        sound_communicate(0); // and send the new values back

        signal(SIGTSTP, sig_tty_parent); /* react to ^Z */

        do {
            soundDevice->borrowTmpData((char*)sound_shared);
            (*soundDevice)();
            if (sound_communicator->update) {
                sound_comm_read(); // this only sets the values
                soundDevice->update(); // really update device
                sound_communicate(0); // and send the new value back
            }
        } while (!sound_communicator->stop);
        CTH_DEBUG("    closing sound reading child.\n");
        delete soundDevice;

        abort();

    default:
        sleep(1); // to make the text output nicer

        // wait, until child is ready
        while (sound_communicator->done_update == 0) {
            sleep(1);
        }

        // get the values from the child
        sound_comm_read();
        sound_communicator->done_update = 0;

        nice(10); // be nice, so that the sound reading proc gets more priority
    }
}

int SoundDeviceFork::read() {

    // get sound data
    memcpy(tmpData, sound_shared, rawSize);
    if (sound_communicator->done_update) {
        sound_comm_read();
        sound_communicator->done_update = 0;
    }
    return rawSize / bytesPerSample;
}

void SoundDeviceFork::update() { sound_communicate(1); }

/*
 * kill the sound reading process
 */
SoundDeviceFork::~SoundDeviceFork() {
    sound_communicator->stop = 1;

    if (sound_child >= 0) {
        CTH_DEBUG("    waiting for sound reading process to stop.\n");
        if (waitpid(sound_child, NULL, WNOHANG) == -1) {
            // first wait failed, wait a short time
            sleep(2);
            CTH_DEBUG("    stopping sound reading process.\n");
            if (waitpid(sound_child, NULL, WNOHANG) == -1) {
                // second wait failed, now be brutal
                kill(SIGKILL, sound_child);
                waitpid(sound_child, NULL, 0);
            }
        }
    }

    shmdt((char*)sound_shared);
    shmctl(sound_shm_key, IPC_RMID, 0);
}
