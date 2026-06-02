// Audio option registry and lifecycle wrapper for the current audio runtime.
// Backend selection enters through AudioRuntime/RuntimeFactory; option changes
// notify the active Audio* source so it can reconfigure device buffers.

#include "cthugha.h"
#include "AudioSystem.h"
#include "AudioOptions.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"
#include "Interface.h"

class OptionAudio : public OptionInt {
public:
    OptionAudio(const char* name, int iV, int maxV = 0, int minV = 0)
        : OptionInt(name, iV, maxV, minV) { }

    virtual void change(int by) {
        OptionInt::change(by);
        audioFrameChange();
    }

    virtual void change(const char* to) { OptionInt::change(to); }
};

class OptionAudioOnOff : public OptionOnOff {
public:
    OptionAudioOnOff(const char* name, int iV)
        : OptionOnOff(name, iV) { }

    virtual void change(int by) {
        OptionOnOff::change(by);
        audioFrameChange();
    }

    virtual void change(const char* to) { OptionOnOff::change(to); }
};

class OptionChannels : public OptionAudio {
public:
    OptionChannels(const char* name, int iV)
        // OptionInt's maximum is exclusive, so this accepts 1 or 2 channels.
        : OptionAudio(name, iV, 3, 1) { }

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

class OptionFormat : public OptionAudio {
    static const char* fmts[];
    static const int nFmts;

public:
    OptionFormat(const char* name, int iV)
        : OptionAudio(name, iV, nFmts) { }

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

int audioInputLoop = 1;

#if WITH_DSP == 1
char dev_dsp[PATH_MAX] = DEV_DSP;
#else
char dev_dsp[PATH_MAX] = "";
#endif

static OptionInt audioInputModeImpl = OptionInt("sound-device-number", AIM_DSPIn, AIM_Max);

static OptionAudio soundSampleRateImpl("sound-sample-rate", 44000);
static OptionChannels soundChannelsImpl("sound-channels", 2);
static OptionFormat soundFormatImpl("sound-format", 0);

static OptionAudio soundDSPMethodImpl("sound-method", 0);
static OptionAudio soundDSPFragmentsImpl("sound-fragments", 16);
static OptionAudio soundDSPFragmentSizeImpl("sound-fragment-size", 0);
static OptionOnOff soundDSPSyncImpl("sound-sync", 0);

static OptionAudioOnOff soundSilentImpl("silent", 0);

Option& audioInputMode = audioInputModeImpl;
Option& soundSampleRate = soundSampleRateImpl;
Option& soundChannels = soundChannelsImpl;
Option& soundFormat = soundFormatImpl;

Option& soundDSPMethod = soundDSPMethodImpl;
Option& soundDSPFragments = soundDSPFragmentsImpl;
Option& soundDSPFragmentSize = soundDSPFragmentSizeImpl;
Option& soundDSPSync = soundDSPSyncImpl;

Option& soundSilent = soundSilentImpl;

InterfaceElement* elementsAudio[] = {
    new InterfaceElementOption("Sample rate       : %10s Hz", &soundSampleRate, 500, 1000, 5000),
    new InterfaceElementOption("Channels          : %10s", &soundChannels),
    new InterfaceElementOption("Format            : %10s", &soundFormat, 1, 1, 1),
#if WITH_DSP == 1
    new InterfaceElementOption("DSP Method        : %10s", &soundDSPMethod),
    new InterfaceElementOption("DSP Fragments     : %10s", &soundDSPFragments),
    new InterfaceElementOption("DSP Fragment Size : %10s", &soundDSPFragmentSize),
    new InterfaceElementOption("DSP Sound sync    : %10s", &soundDSPSync),
#endif
};
int nElementsAudio = sizeof(elementsAudio) / sizeof(InterfaceElement*);

Interface interfaceAudio("sound", "Audio Interface", NULL, elementsAudio, nElementsAudio);

int init_sound(int visualMaxDimension) {
    if (audioRuntimeIsInitialized())
        return 0;

    return audioRuntimeInit(1, visualMaxDimension);
}

int exit_sound() {
    audioRuntimeShutdown();
    return 0;
}

Interface interfacePlayList("playList", "Sound Play List", "Not yet implemented");
