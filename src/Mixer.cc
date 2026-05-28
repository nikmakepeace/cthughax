#include "cthugha.h"
#include "Mixer.h"
#include "Interface.h"

#if WITH_MIXER == 1

//
// Mixer enabled
//

// include the right soundcard.h
#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class OptionVolume : public OptionInt {
protected:
    char name[32];
    int devNr;
    int active;

public:
    OptionVolume()
        : OptionInt(0, 0)
        , devNr(0)
        , active(0) { }

    void setName(const char* n) { strncpy(name, n, 32); }

    virtual void setValue(int value);
    virtual void change(int by);
    virtual const char* text() const;

    friend int init_mixer();
};

char dev_mixer[PATH_MAX] = DEV_MIXER;

static const char* mixer_names[] = SOUND_DEVICE_NAMES;

static int mixer_initials = 0; // any initial values set
static int mixer_initial[SOUND_MIXER_NRDEVICES]; // initial value for mixer device
                                                 // -1: don't change initial value
static OptionVolume volumes[SOUND_MIXER_NRDEVICES];
static int nVolumes = 0;

const char* OptionVolume::text() const {
    static char str[128];

    sprintf(str, "%3d:%-3d%c", value % 256, value / 256, active ? '*' : ' ');
    return str;
}

void OptionVolume::setValue(int value_) {
    OptionInt::setValue((value_ < 255) ? value_ * 256 + value_ : value_);
}

void OptionVolume::change(int by) {

    int left = value % 256;
    left += by;
    if (left < 0)
        left = 0;
    if (left > 255)
        left = 255;

    int right = value / 256;
    right += by;
    if (right < 0)
        right = 0;
    if (right > 255)
        right = 255;

    value = left + 256 * right;

    int mixer_des;
    if ((mixer_des = open(dev_mixer, O_RDWR)) < 0) {
        CTH_ERRNO(errno, "Can not open `%s'.", dev_mixer);
    }

    // get, which recording devices are active for recording
    int mixer_mask = 0;
    if (ioctl(mixer_des, MIXER_READ(SOUND_MIXER_RECSRC), &mixer_mask) < 0) {
        CTH_ERRNO(errno, "Can not get recording source mask.");
    }

    // update mixer mask
    if (value <= 0) {
        value = 0;
        mixer_mask &= ~(1 << devNr);
    } else {
        mixer_mask |= (1 << devNr);
    }

    // set device mask
    if (ioctl(mixer_des, MIXER_WRITE(SOUND_MIXER_RECSRC), &mixer_mask) < 0) {
        CTH_ERRNO(errno, "Can not set recording source mask");
    }
    // and read it again
    if (ioctl(mixer_des, MIXER_READ(SOUND_MIXER_RECSRC), &mixer_mask) < 0) {
        CTH_ERRNO(errno, "Can not get recording source mask.");
    }
    // to set the active mark
    active = mixer_mask & (1 << devNr);

    // set volume
    if (ioctl(mixer_des, MIXER_WRITE(devNr), &value) < 0) {
        CTH_ERRNO(errno, "Can not set mixer value for `%s'.", name);
    }

    close(mixer_des);
}

//
// set an initial volume for a mixer device
//
// called while parsing options and ini files
//
int mixer_initial_volume(char* name, int volume) {

    if (mixer_initials == 0) {
        // init initial volumes
        for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
            mixer_initial[i] = -1;

        mixer_initials = 1;
    }

    // find device and remember initial value
    for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
        if (strcasecmp(mixer_names[i], name) == 0) {
            mixer_initial[i] = volume;
            return 0;
        }
    }

    CTH_ERROR("unknown mixer device `%s'.\n", name);
    return 1;
}

//
// initialize mixer
//
// check for availalble devices
// set the initial volumes
//
int init_mixer() {
    int mixer_des;

    // check if there is any initial value
    if (mixer_initials == 0) {
        CTH_DEBUG("  no initial volumes specified.\n");

        // init initial volumes in that case
        for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++)
            mixer_initial[i] = -1;
    }

    nVolumes = 0;

    if ((mixer_des = open(dev_mixer, O_RDONLY)) < 0) {
        if ((errno == ENOENT) || (errno == ENODEV)) {
            if (mixer_initials) {
                CTH_WARN("  OSS mixer `%s' is unavailable; mixer options will be ignored.\n",
                    dev_mixer);
            } else {
                CTH_DEBUG("  OSS mixer `%s' is unavailable; skipping mixer initialization.\n",
                    dev_mixer);
            }
            return 0;
        }
        CTH_ERRNO(errno, "Can not open `%s'.", dev_mixer);
        return 0;
    }

    /* get, which features are actually available */
    int dev_mask = 0;
    if (ioctl(mixer_des, SOUND_MIXER_READ_DEVMASK, &dev_mask) < 0) {
        CTH_ERRNO(errno, "Can not get mixer device mask.");
    }

    // get, which recording devices are active for recording
    int mixer_mask = 0;
    if (ioctl(mixer_des, MIXER_READ(SOUND_MIXER_RECSRC), &mixer_mask) < 0) {
        CTH_ERRNO(errno, "Can not get recording source mask.");
    }

    interfaceMixer.elements = new InterfaceElement*[SOUND_MIXER_NRDEVICES];

    for (int i = 0; i < SOUND_MIXER_NRDEVICES; i++) {

        // check if device is available
        if ((dev_mask & (1 << i)) == 0) {

            if (mixer_initial[i] != -1) {
                CTH_WARN("  unavailable mixer device `%s'.\n", mixer_names[i]);
            }

        } else {

            if (mixer_initial[i] == -1) {
                // get initial volume, if not set
                if (ioctl(mixer_des, MIXER_READ(i), &(mixer_initial[i])) < 0) {
                    CTH_ERRNO(errno, "Can not get mixer value for `%s'.", mixer_names[i]);
                }
            } else {
                if (mixer_initial[i] == 0) {
                    // disable device
                    mixer_mask &= ~(1 << i);
                    CTH_DEBUG("    disabling `%s' for input.\n", mixer_names[i]);
                } else {
                    CTH_DEBUG("    setting `%s' to volume %d.\n", mixer_names[i],
                        mixer_initial[i] & 255);
                    mixer_mask |= 1 << i;
                }

                // set device volume
                if (ioctl(mixer_des, MIXER_WRITE(i), &(mixer_initial[i])) < 0) {
                    CTH_ERRNO(errno, "Can not set mixer value for `%s'.", mixer_names[i]);
                }
            }
            // keep everything in the option
            volumes[nVolumes].setName(mixer_names[i]);
            volumes[nVolumes].devNr = i;
            volumes[nVolumes].setValue(mixer_initial[i]);
            volumes[nVolumes].active = mixer_mask & (1 << i);

            // add it to the interface
            char* str = new char[128];
            sprintf(str, "%10s       : %%s", mixer_names[i]);
            interfaceMixer.elements[nVolumes]
                = new InterfaceElementOption(str, &volumes[nVolumes], 1, 10, 255);
            interfaceMixer.nElements++;
            nVolumes++;
        }
    }

    // set device mask
    if (ioctl(mixer_des, MIXER_WRITE(SOUND_MIXER_RECSRC), &mixer_mask) < 0) {
        CTH_ERRNO(errno, "Can not set recording source");
    }

    close(mixer_des);

    CTH_TRACE("available mixer devices: %d.\n", "mixer", nVolumes);

    return 0;
}

#else

//
// mixer disabled
//

char dev_mixer[PATH_MAX] = "";

// dummy initialization
int init_mixer() { return 0; }

int mixer_initial_volume(char* /*name*/, int /*volume*/) {
    CTH_WARN("mixer was disabled at compile time.\n");
    return 0;
}

#endif
