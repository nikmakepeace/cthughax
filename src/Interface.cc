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
#include "Scene.h"
#include "VideoDirector.h"
#include "flames.h"
#include "translate.h"
#include "waves.h"

#include <ctype.h>
#include <signal.h>

int Interface::saveToHot = 0;
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
    , sel(-1)
    , silenceMsg(NULL)
    , silenceLine(0) {

    next = head; // add into list of interfaces
    head = this;
}

Interface::Interface(const char* n, const char* ti, const char* te, InterfaceElement* el[], int nEl)
    : name(n)
    , title(ti)
    , text(te)
    , elements(el)
    , nElements(nEl)
    , sel(-1)
    , silenceMsg(NULL)
    , silenceLine(0) {

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

ACTION(chgValue1) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc1);
    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (currentCoreOption && sceneCommands != NULL
        && sceneCommands->isSceneOption(*currentCoreOption))
        sceneCommands->change(*currentCoreOption, step, 0);
    else if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(chgValue2) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc2);
    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (currentCoreOption && sceneCommands != NULL
        && sceneCommands->isSceneOption(*currentCoreOption))
        sceneCommands->change(*currentCoreOption, step, 0);
    else if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(chgValue3) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc3);
    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (currentCoreOption && sceneCommands != NULL
        && sceneCommands->isSceneOption(*currentCoreOption))
        sceneCommands->change(*currentCoreOption, step, 0);
    else if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(setValue) {
    if (currentOptionInterfaceElement == NULL)
        return;

    char str[128];
    sprintf(str, "%d", int(v * currentOptionInterfaceElement->inc1));

    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (currentCoreOption && sceneCommands != NULL
        && sceneCommands->isSceneOption(*currentCoreOption)) {
        sceneCommands->change(*currentCoreOption, str, 0);
        sceneCommands->change(*currentCoreOption, 0, 0);
    } else if (currentCoreOption) {
        currentCoreOption->change(str, 0);
        currentCoreOption->change(0, 0);
    } else if (currentOption) {
        currentOption->change(str);
        currentOption->change(0);
    }
}

static const char* InterfaceList[] = { "Help", // F1
    "CoreOptions", // F2
    "Options", // F3
#if WITH_CDROM == 1
    "CD", // F4
#endif
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
        sprintf(str, "%s%s", (cthughaDisplay != NULL) ? cthughaDisplay->status() : "",
            (autoChanger != NULL) ? autoChanger->status() : "");

        displayDevice->print(str, text_size.y - 1, 'l', TEXT_COLOR_NORMAL, 1);
    }

    if (saveToHot) {
        displayDevice->print("save to hotkey (press 0..9)", text_size.y - (showStatus ? 2 : 1), 'l',
            TEXT_COLOR_NORMAL);
    }

    if (audioMetrics.noisy) {
        silenceMsg = NULL;
    } else if ((silenceMsg != NULL) && (nElements == 0))
        displayDevice->print(silenceMsg, silenceLine, 'c', TEXT_COLOR_NORMAL);
}

void Interface::msg(char* msg) {
    silenceMsg = msg;
    silenceLine = rand() % (text_size.y - 5);
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

    sprintf(fmt, "%%c%%-%ds%%c", min(text_size.x - 3, 77));
    sprintf(in, str, opt->text());

    // make format and include the > <
    sprintf(strRet, fmt, selected ? '>' : ' ', in, selected ? '<' : ' ');

    return strRet;
}

Option* currentOption = NULL;
CoreOption* currentCoreOption = NULL;
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
// Core Option Element
//
Keymap InterfaceElementCoreOption::coreKeymap("CoreOptionElement");

InterfaceElementCoreOption::InterfaceElementCoreOption(
    const char* t, CoreOption* o, int i1, int i2, int i3)
    : InterfaceElementOption(t, o, i1, i2, i3)
    , coreOpt(o) { }

int InterfaceElementCoreOption::doKey(int key) {

    currentCoreOption = coreOpt;
    currentOption = coreOpt;
    currentOptionInterfaceElement = this;

    int ret = coreKeymap.action(key);
    if (ret == 1)
        ret = keymap.action(key);

    currentOption = NULL;
    currentCoreOption = NULL;
    currentOptionInterfaceElement = NULL;

    return ret;
}

ACTION(lockElement) {
    if (currentCoreOption)
        currentCoreOption->lock.change(+1);
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

static const char* sceneOptionText(SceneOptionText field, CoreOption* fallback) {
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

class InterfaceElementSceneCoreOption : public InterfaceElementCoreOption {
    SceneOptionText sceneText;

public:
    InterfaceElementSceneCoreOption(const char* t, CoreOption* o, SceneOptionText sceneText_)
        : InterfaceElementCoreOption(t, o)
        , sceneText(sceneText_) { }

    virtual const char* text(int selected) {
        static char strRet[512];
        char fmt[512];
        char in[512];

        sprintf(fmt, "%%c%%-%ds%%c", min(text_size.x - 3, 77));
        sprintf(in, str, sceneOptionText(sceneText, coreOpt));
        sprintf(strRet, fmt, selected ? '>' : ' ', in, selected ? '<' : ' ');

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
    const int errorTime = 300;
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

class InterfaceCoreOption : public Interface {
public:
    InterfaceCoreOption()
        : Interface("CoreOptions", "Core Options", NULL) {

        nElements = 13;
        elements = new InterfaceElement*[nElements];

        {
            elements[0] = new InterfaceElementCoreOption("Display (d,D)         : %s", &screen);
            elements[1] = new InterfaceElementSceneCoreOption("Flame (f,F)           : %s",
                &flame, SceneOptionFlame);
            elements[2]
                = new InterfaceElementSceneCoreOption("General Flame (g)     : %s",
                    &flameGeneral, SceneOptionGeneralFlame);
            elements[3] = new InterfaceElementSceneCoreOption("Border (=)            : %s",
                &border, SceneOptionBorder);
            elements[4]
                = new InterfaceElementSceneCoreOption("Translate (t,T)       : %s",
                    &translation, SceneOptionTranslate);
            elements[5] = new InterfaceElementSceneCoreOption("Wave (w)              : %s",
                &wave, SceneOptionWave);
            elements[6]
                = new InterfaceElementOption("Sound Processing (m,M): %s", &audioProcessing);
            elements[7] = new InterfaceElementSceneCoreOption("Table (b,B)           : %s",
                &table, SceneOptionTable);
            elements[8] = new InterfaceElementSceneCoreOption("WaveScale (W)         : %s",
                &waveScale, SceneOptionWaveScale);
            elements[9]
                = new InterfaceElementSceneCoreOption("Palette (p,P)         : %s",
                    &palette, SceneOptionPalette);
            elements[10]
                = new InterfaceElementCoreOption("Image (x,X))          : %s",
                    &videoDirector().imageOption());
            elements[11] = new InterfaceElementSceneCoreOption("3D-Object (j,J)       : %s",
                &object, SceneOptionObject);
            elements[12]
                = new InterfaceElementSceneCoreOption("Flashlight (s)        : %s",
                    &flashlight, SceneOptionFlashlight);
        }
    }

    void preRun() {
#define O(i)                                                                                       \
    ((InterfaceElementOption*)elements[i])->opt                                                    \
        = ((InterfaceElementCoreOption*)elements[i])->coreOpt
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
        if (Keymap::action("CoreOptions", key) == 0)
            return;
        if (Keymap::action("Options", key) == 0)
            return;
        Keymap::action("default", key);
    }

} interfaceCoreOption;

InterfaceElement* elementsOption[] = {
    new InterfaceElementOption("Maximal Frames/second    : %10s", &maxFramesPerSecond),
    new InterfaceElementOption("Zoom (0=max)             : %10s", &zoom),
    new InterfaceElementOption("Minimal time btw. change : %10s", &changeWaitMin, 10, 50, 100),
    new InterfaceElementOption("Extra random time        : %10s", &changeWaitRandom, 10, 50, 100),
    new InterfaceElementOption("Quiet change time        : %10s", &changeQuiet, 10, 50, 100),
    new InterfaceElementOption("Time before silence Msg. : %10s", &changeMsgTime, 10, 50, 100),
    new InterfaceElementOption("Minimal noise level      : %10s", &sound_minnoise, 1, 10, 255),
    new InterfaceElementOption("Cumulative fire level    : %10s", &changeCumulativeFireLevel, 10, 50, 100),
    new InterfaceElementOption("Little changes only      : %10s", &change_little),
    new InterfaceElementOption("Lock                     : %10s", &lock),
};
int nElementsOption = sizeof(elementsOption) / sizeof(InterfaceElement*);

Interface interfaceOption("Options", "Options", NULL, elementsOption, nElementsOption);
