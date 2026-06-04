#include "cthugha.h"
#include "Interface.h"
#include "keys.h"
#include "imath.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "AutoChanger.h"
#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "Border.h"
#include "Flashlight.h"
#include "RuntimeCommandSink.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "flames.h"
#include "TranslationOptions.h"
#include "waves.h"

#include <ctype.h>
#include <signal.h>

int Interface::saveToPreset = 0;
int Interface::showStatus = 0;

Interface* Interface::current = NULL;

Interface* Interface::head = NULL;

////////////////////////////////////////////////////////////////////////////

//
// class Interface
//
Interface::Interface(const char* n, const char* ti, const char* te)
    : name(n)
    , title(ti)
    , text(te)
    , elements(NULL)
    , nElements(0)
    , sel(-1) {

    next = head; // add into list of interfaces
    head = this;
}

Interface::Interface(const char* n, const char* ti, const char* te, InterfaceElement* el[], int nEl)
    : name(n)
    , title(ti)
    , text(te)
    , elements(el)
    , nElements(nEl)
    , sel(-1) {

    next = head; // add into list of interfaces
    head = this;
}

Interface::~Interface() { }

void Interface::setElements(InterfaceElement** el, int nEl) {
    elements = el;
    nElements = nEl;
}

void Interface::set(const char* n) {

    for (Interface* i = head; i != NULL; i = i->next)
        if (strcasecmp(n, i->name) == 0) {
            current = i;
            return;
        }
    // could not find it, leave it as it is
    CTH_ERROR("Unknown interface '%s'\n", n);
}

ErrorMessages errors;

ACTION(up) {
    if (Interface::current == NULL)
        return;

    Interface::current->sel -= int(v);
    if (Interface::current->sel < -1)
        Interface::current->sel = -1;
    if (Interface::current->sel >= Interface::current->nElements)
        Interface::current->sel = Interface::current->nElements - 1;
}

ACTION(down) {
    if (Interface::current == NULL)
        return;

    Interface::current->sel += int(v);
    if (Interface::current->sel < -1)
        Interface::current->sel = -1;
    if (Interface::current->sel >= Interface::current->nElements)
        Interface::current->sel = Interface::current->nElements - 1;
}

ACTION(home) {
    if (Interface::current == NULL)
        return;

    Interface::current->sel = -1;
}
ACTION(end) {
    if (Interface::current == NULL)
        return;

    Interface::current->sel = Interface::current->nElements - 1;
}

static void changeCurrentInterfaceValueBy(int step) {
    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (sink == NULL)
        return;

    if (currentEffectControl) {
        sink->apply(RuntimeCommand::changeEffectControlBy(*currentEffectControl, step));
    } else if (currentOption) {
        sink->apply(RuntimeCommand::changeOptionBy(*currentOption, step));
    }
}

static void changeCurrentInterfaceValueTo(const char* text) {
    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (sink == NULL)
        return;

    if (currentEffectControl) {
        sink->apply(RuntimeCommand::changeEffectControlTo(*currentEffectControl, text));
        sink->apply(RuntimeCommand::changeEffectControlBy(*currentEffectControl, 0));
    } else if (currentOption) {
        sink->apply(RuntimeCommand::changeOptionTo(*currentOption, text));
        sink->apply(RuntimeCommand::changeOptionBy(*currentOption, 0));
    }
}

ACTION(chgValue1) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc1);
    changeCurrentInterfaceValueBy(step);
}
ACTION(chgValue2) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc2);
    changeCurrentInterfaceValueBy(step);
}
ACTION(chgValue3) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc3);
    changeCurrentInterfaceValueBy(step);
}
ACTION(setValue) {
    if (currentOptionInterfaceElement == NULL)
        return;

    char str[128];
    snprintf(str, sizeof(str), "%d", int(v * currentOptionInterfaceElement->inc1));
    changeCurrentInterfaceValueTo(str);
}

static const char* InterfaceList[] = { "Help", // F1
    "EffectControls", // F2
    "Options", // F3
    "sound", // F5
    "mixer", // F6
    "playList", // F7
    "playList", // F8

    "display", // F9
    "flame", "border", "translate", "wave", "table", "waveScaling", "object",
    "palette", "image", "flashlight", "Help", NULL };

ACTION(nextInterface) {
    for (const char** N = InterfaceList; *N != NULL; N++)
        if (strcasecmp(*N, Interface::current->name) == 0) {
            Interface::set(N[1]);
            return;
        }
}
ACTION(prevInterface) {
    for (const char** N = InterfaceList + 1; *N != NULL; N++)
        if (strcasecmp(*N, Interface::current->name) == 0) {
            Interface::set(N[-1]);
            return;
        }
}

// default display handler
void Interface::display() {
    double line = 0.0;

    if (title) {
        line = displayDevice->print(
            title, 0, 'l', (sel == -1) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
        line = displayDevice->print(
            "---------------------------------------------", line, 'l', TEXT_COLOR_NORMAL);
    }

    if (text) {
        line = displayDevice->print(text, line, 'l', TEXT_COLOR_NORMAL);
    }

    for (int i = 0; i < nElements; i++) {
        line = displayDevice->print(elements[i]->text(sel == i), line, 'l',
            (sel == i) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
    }

    if (showStatus) {
        static char str[512];
        snprintf(str, sizeof(str), "%s%s", (cthughaDisplay != NULL) ? cthughaDisplay->status() : "",
            (autoChanger != NULL) ? autoChanger->status() : "");

        displayDevice->print(str, text_size.y - 1, 'l', TEXT_COLOR_NORMAL, 1);
    }

    if (saveToPreset) {
        displayDevice->print("save to preset slot (press 0..9)", text_size.y - (showStatus ? 2 : 1), 'l',
            TEXT_COLOR_NORMAL);
    }

}

void Interface::doKey(int key) {

    if (Keymap::action(name, key) == 0)
        return;
    Keymap::action("default", key);
}

// default runner
void Interface::run() {

    this->preRun();

    // handle keys
    int key;
    while ((key = getkey()) != CK_NONE) {

        if ((sel >= 0) && (sel < nElements))
            if (elements[sel]->doKey(key) == 0)
                continue;

        this->doKey(key);
    }
}

//
// class InterfaceElementOption
//

Keymap InterfaceElementOption::keymap("OptionElement");

InterfaceElementOption::InterfaceElementOption(const char* t, Option* o, int i1, int i2, int i3)
    : InterfaceElement(t)
    , opt(o)
    , inc1(i1)
    , inc2(i2)
    , inc3(i3) { }

const char* InterfaceElementOption::text(int selected) {
    static char strRet[512];
    char fmt[512];
    char in[512];

    snprintf(fmt, sizeof(fmt), "%%c%%-%ds%%c", min(text_size.x - 3, 77));
    snprintf(in, sizeof(in), str, opt->text());

    // make format and include the > <
    snprintf(strRet, sizeof(strRet), fmt, selected ? '>' : ' ', in, selected ? '<' : ' ');

    return strRet;
}

Option* currentOption = NULL;
EffectControl* currentEffectControl = NULL;
InterfaceElementOption* currentOptionInterfaceElement = NULL;

int InterfaceElementOption::doKey(int key) {

    currentOption = opt;
    currentOptionInterfaceElement = this;
    int ret = keymap.action(key);
    currentOption = NULL;
    currentOptionInterfaceElement = NULL;
    return ret;
}

//
// Effect Control Element
//
Keymap InterfaceElementEffectControl::effectControlKeymap("EffectControlElement");

InterfaceElementEffectControl::InterfaceElementEffectControl(
    const char* t, EffectControl* o, int i1, int i2, int i3)
    : InterfaceElementOption(t, o, i1, i2, i3)
    , effectControl(o) { }

int InterfaceElementEffectControl::doKey(int key) {

    currentEffectControl = effectControl;
    currentOption = effectControl;
    currentOptionInterfaceElement = this;

    int ret = effectControlKeymap.action(key);
    if (ret == 1)
        ret = keymap.action(key);

    currentOption = NULL;
    currentEffectControl = NULL;
    currentOptionInterfaceElement = NULL;

    return ret;
}

ACTION(lockElement) {
    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (currentEffectControl && sink != NULL)
        sink->apply(RuntimeCommand::toggleEffectControlLock(*currentEffectControl));
}

enum SceneOptionText {
    SceneOptionFlame,
    SceneOptionGeneralFlame,
    SceneOptionBorder,
    SceneOptionTranslate,
    SceneOptionWave,
    SceneOptionTable,
    SceneOptionWaveScale,
    SceneOptionPalette,
    SceneOptionObject,
    SceneOptionFlashlight
};

static const char* sceneOptionText(SceneOptionText field, EffectControl* fallback) {
    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (sceneCommands == NULL)
        return fallback->text();

    const SceneSettings& settings = sceneCommands->sceneState().settings();
    switch (field) {
    case SceneOptionFlame:
        return settings.flameName;
    case SceneOptionGeneralFlame:
        return settings.generalFlameName;
    case SceneOptionBorder:
        return settings.borderName;
    case SceneOptionTranslate:
        return settings.translationName;
    case SceneOptionWave:
        return settings.waveName;
    case SceneOptionTable:
        return settings.tableName;
    case SceneOptionWaveScale:
        return settings.waveScaleName;
    case SceneOptionPalette:
        return settings.paletteName;
    case SceneOptionObject:
        return settings.objectName;
    case SceneOptionFlashlight:
        return settings.flashlightName;
    }

    return fallback->text();
}

class InterfaceElementSceneEffectControl : public InterfaceElementEffectControl {
    SceneOptionText sceneText;

public:
    InterfaceElementSceneEffectControl(const char* t, EffectControl* o, SceneOptionText sceneText_)
        : InterfaceElementEffectControl(t, o)
        , sceneText(sceneText_) { }

    virtual const char* text(int selected) {
        static char strRet[512];
        char fmt[512];
        char in[512];

        snprintf(fmt, sizeof(fmt), "%%c%%-%ds%%c", min(text_size.x - 3, 77));
        snprintf(in, sizeof(in), str, sceneOptionText(sceneText, effectControl));
        snprintf(strRet, sizeof(strRet), fmt, selected ? '>' : ' ', in, selected ? '<' : ' ');

        return strRet;
    }
};

void ErrorMessages::addMessage(const char* text) {
    if (nMsgs == 128) {
        CTH_ERROR("too many errors: %s\n", text);
        return;
    }
    strncpy(msgs[nMsgs], text, 128);
    on_screen[nMsgs] = gettime();

    nMsgs++;
}
void ErrorMessages::display() {

    // bring messages to screen
    for (int i = 0; i < nMsgs; i++) {
        displayDevice->print(msgs[i], -(nMsgs - i), 'r', TEXT_COLOR_ERROR);
    }

    // remove old messages
    const int errorTime = 3000;
    while ((nMsgs > 0) && ((gettime() - on_screen[0]) > errorTime)) {
        for (int i = 1; i < nMsgs; i++) {
            strncpy(msgs[i - 1], msgs[i], 128);
            on_screen[i - 1] = on_screen[i];
        }
        nMsgs--;
    }
}

//
// the interfaces
//
class InterfaceMain : public Interface {
    char extraKeymap[512];

public:
    InterfaceMain()
        : Interface("main", NULL, NULL) {
        extraKeymap[0] = '\0';
    }

    void doKey(int key) {
        if (extraKeymap[0] != '\0') {
            if (Keymap::action(extraKeymap, key) == 0)
                return;
        }
        Interface::doKey(key);
    }

    friend class setExtraKeymapAction;
} interfaceMain;

ACTION(setExtraKeymap) { strncpy(interfaceMain.extraKeymap, p, 512); }

Interface interfaceMixer("Mixer", "Mixer", NULL);

class InterfaceEffectControl : public Interface {
public:
    InterfaceEffectControl()
        : Interface("EffectControls", "Effect Controls", NULL) {

        nElements = 13;
        elements = new InterfaceElement*[nElements];

        {
            elements[0] = new InterfaceElementEffectControl("Display (d,D)         : %s", &screen);
            elements[1] = new InterfaceElementSceneEffectControl("Flame (f,F)           : %s",
                &flame, SceneOptionFlame);
            elements[2]
                = new InterfaceElementSceneEffectControl("General Flame (g)     : %s",
                    &flameGeneral, SceneOptionGeneralFlame);
            elements[3] = new InterfaceElementSceneEffectControl("Border (=)            : %s",
                &border, SceneOptionBorder);
            elements[4]
                = new InterfaceElementSceneEffectControl("Translate (t,T)       : %s",
                    &translation, SceneOptionTranslate);
            elements[5] = new InterfaceElementSceneEffectControl("Wave (w)              : %s",
                &wave, SceneOptionWave);
            elements[6]
                = new InterfaceElementOption("Sound Processing (m,M): %s", &audioProcessing);
            elements[7] = new InterfaceElementSceneEffectControl("Table (b,B)           : %s",
                &table, SceneOptionTable);
            elements[8] = new InterfaceElementSceneEffectControl("WaveScale (W)         : %s",
                &waveScale, SceneOptionWaveScale);
            elements[9]
                = new InterfaceElementSceneEffectControl("Palette (p,P)         : %s",
                    &palette, SceneOptionPalette);
            elements[10]
                = new InterfaceElementEffectControl("Image (x,X))          : %s",
                    &videoDirector().imageOption());
            elements[11] = new InterfaceElementSceneEffectControl("3D-Object (j,J)       : %s",
                &object, SceneOptionObject);
            elements[12]
                = new InterfaceElementSceneEffectControl("Flashlight (s)        : %s",
                    &flashlight, SceneOptionFlashlight);
        }
    }

    void preRun() {
#define O(i)                                                                                       \
    ((InterfaceElementOption*)elements[i])->opt                                                    \
        = ((InterfaceElementEffectControl*)elements[i])->effectControl
        O(1) = &flame;
        O(2) = &flameGeneral;
        O(3) = &border;
        O(4) = &translation;
        O(5) = &wave;
        O(7) = &table;
        O(8) = &waveScale;
        O(9) = &palette;
        O(10) = &videoDirector().imageOption();
        O(11) = &object;
        O(12) = &flashlight;
    }
    void doKey(int key) {
        if (Keymap::action("EffectControls", key) == 0)
            return;
        if (Keymap::action("Options", key) == 0)
            return;
        Keymap::action("default", key);
    }

} interfaceEffectControl;

InterfaceElement* elementsOption[] = {
    new InterfaceElementOption("Maximal Frames/second    : %10s", &maxFramesPerSecond),
    new InterfaceElementOption("Zoom (0=max)             : %10s", &zoom),
    new InterfaceElementOption("Minimal time btw. change : %10s", &changeWaitMin, 100, 500, 1000),
    new InterfaceElementOption("Extra random time        : %10s", &changeWaitRandom, 100, 500, 1000),
    new InterfaceElementOption("Quiet change time        : %10s", &changeQuiet, 100, 500, 1000),
    new InterfaceElementOption("Time before silence Msg. : %10s", &changeMsgTime, 100, 500, 1000),
    new InterfaceElementOption("Minimal noise level      : %10s", &sound_minnoise, 1, 10, 255),
    new InterfaceElementOption("Cumulative fire level    : %10s", &changeCumulativeFireLevel, 10, 50, 100),
    new InterfaceElementOption("Little changes only      : %10s", &change_little),
    new InterfaceElementOption("Lock                     : %10s", &lock),
};
int nElementsOption = sizeof(elementsOption) / sizeof(InterfaceElement*);

Interface interfaceOption("Options", "Options", NULL, elementsOption, nElementsOption);
