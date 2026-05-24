// Sound option registry and lifecycle wrapper for the current audio frame.
// Backend selection now enters through AudioRuntime/RuntimeFactory; option
// changes notify the active backend so it can reconfigure device buffers.

#include "cthugha.h"
#include "Sound.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"
#include "display.h"
#include "information.h"
#include "Interface.h"
#include "imath.h"
#include "cth_buffer.h"

class OptionSound : public OptionInt {
protected:
public:
    OptionSound(const char* name, int iV, int maxV = 0, int minV = 0)
        : OptionInt(name, iV, maxV, minV) { }

    virtual void change(int by) {
        OptionInt::change(by);

        audioFrameChange();

        bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    }
    virtual void change(const char* to) { OptionInt::change(to); }
};

class OptionSoundOnOff : public OptionOnOff {
public:
    OptionSoundOnOff(const char* name, int iV)
        : OptionOnOff(name, iV) { }

    virtual void change(int by) {
        OptionOnOff::change(by);

        audioFrameChange();

        bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    }
    virtual void change(const char* to) { OptionOnOff::change(to); }
};

class OptionChannels : public OptionSound {
protected:
public:
    OptionChannels(const char* name, int iV)
        // OptionInt's maximum is exclusive, so this accepts 1 or 2 channels.
        : OptionSound(name, iV, 3, 1) { }

    const char* text() const {
        switch (value) {
        case 1:
            return "mono";
        case 2:
            return "stereo";
        default:
            return "unknown";
        }
    }
};

class OptionFormat : public OptionSound {
protected:
    static const char* fmts[];
    static const int nFmts;

public:
    OptionFormat(const char* name, int iV)
        : OptionSound(name, iV, nFmts) { }

    virtual void change(const char* to) {

        for (value = 0; value < nFmts; value++)
            if (strcasecmp(fmts[value], to) == 0)
                return;

        OptionInt::change(to);
    }

    const char* text() const {
        if ((value >= 0) && (value < nFmts))
            return fmts[value];
        return "unknown";
    }
};
const char* OptionFormat::fmts[] = {
    "8bit unsigned",
    "8bit signed",
    "16bit unsigned (le)",
    "16bit signed (le)",
    "16bit unsigned (be)",
    "16bit signed (be)",
};
const int OptionFormat::nFmts = sizeof(OptionFormat::fmts) / sizeof(const char*);

SoundDevice* soundDevice = NULL;

static OptionInt soundDeviceNrImpl = OptionInt("sound-device-number", SDN_DSPIn, SDN_Max);
static OptionOnOff soundForkImpl("sound-fork", 0);

static OptionSound soundSampleRateImpl("sound-sample-rate", 44000);
static OptionChannels soundChannelsImpl("sound-channels", 2);
static OptionFormat soundFormatImpl("sound-format", 0);

static OptionSound soundDSPMethodImpl("sound-method", 0);
static OptionSound soundDSPFragmentsImpl("sound-fragments", 16);
static OptionSound soundDSPFragmentSizeImpl("sound-fragment-size", 0);
static OptionOnOff soundDSPSyncImpl("sound-sync", 0);

static OptionSound soundBufferImpl("sound-buffer", 1024);
static OptionSoundOnOff soundSilentImpl("silent", 0);

Option& soundDeviceNr = soundDeviceNrImpl;
Option& soundFork = soundForkImpl;
Option& soundSampleRate = soundSampleRateImpl;
Option& soundChannels = soundChannelsImpl;
Option& soundFormat = soundFormatImpl;

Option& soundDSPMethod = soundDSPMethodImpl;
Option& soundDSPFragments = soundDSPFragmentsImpl;
Option& soundDSPFragmentSize = soundDSPFragmentSizeImpl;
Option& soundDSPSync = soundDSPSyncImpl;

Option& soundBuffer = soundBufferImpl;
Option& soundSilent = soundSilentImpl;

InterfaceElement* elementsSound[] = {
    new InterfaceElementOption("Sample rate       : %10s Hz", &soundSampleRate, 500, 1000, 5000),
    new InterfaceElementOption("Channels          : %10s", &soundChannels),
    new InterfaceElementOption("Format            : %10s", &soundFormat, 1, 1, 1),
#if WITH_DSP == 1
    new InterfaceElementOption("DSP Method        : %10s", &soundDSPMethod),
    new InterfaceElementOption("DSP Fragments     : %10s", &soundDSPFragments),
    new InterfaceElementOption("DSP Fragment Size : %10s", &soundDSPFragmentSize),
    new InterfaceElementOption("DSP Sound sync    : %10s", &soundDSPSync),
#endif
    new InterfaceElementOption("File buffer size  : %10s k", &soundBuffer),
};
int nElementsSound = sizeof(elementsSound) / sizeof(InterfaceElement*);

Interface interfaceSound("sound", "Sound Interface", NULL, elementsSound, nElementsSound);

int init_sound() {

    if (soundDevice != NULL)
        return 0;

    audioRuntimeInit(RSIC_MainProcess, 1);

    return 0;
}

int exit_sound() {
    audioRuntimeShutdown();
    return 0;
}

Interface interfacePlayList("playList", "Sound Play List", "Not yet implemented");
