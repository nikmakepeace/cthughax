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
    if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(chgValue2) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc2);
    if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(chgValue3) {
    if (currentOptionInterfaceElement == NULL)
        return;

    int step = int(v * currentOptionInterfaceElement->inc3);
    if (currentCoreOption)
        currentCoreOption->change(step, 0);
    else if (currentOption)
        currentOption->change(step);
}
ACTION(setValue) {
    if (currentOptionInterfaceElement == NULL)
        return;

    char str[128];
    sprintf(str, "%d", int(v * currentOptionInterfaceElement->inc1));

    if (currentCoreOption) {
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
    "palette", "pcx", "flashlight", "light", "background", "fly", "Help", NULL };

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
        sprintf(str, "%s%s%s", (cthughaDisplay != NULL) ? cthughaDisplay->status() : "",
            (autoChanger != NULL) ? autoChanger->status() : "",
            (CthughaBuffer::current != NULL) ? CthughaBuffer::current->translate.status() : "");

        displayDevice->print(str, text_size.y - 1, 'l', TEXT_COLOR_NORMAL, 1);
    }

    if (saveToHot) {
        displayDevice->print("save to hotkey (press 0..9)", text_size.y - (showStatus ? 2 : 1), 'l',
            TEXT_COLOR_NORMAL);
    }

    if (audioAnalysis.noisy) {
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

        nElements = 17;
        elements = new InterfaceElement*[nElements];

        CthughaBuffer& b = CthughaBuffer::buffers[CthughaBuffer::nCurrent];

        elements[0]
            = new InterfaceElementOption("Current Buffer (<,>)  : %s", &CthughaBuffer::nCurrent);

        {
            elements[1] = new InterfaceElementCoreOption("Display (d,D)         : %s", &screen);
            elements[2] = new InterfaceElementCoreOption("Flame (f,F)           : %s", &b.flame);
            elements[3]
                = new InterfaceElementCoreOption("General Flame (g)     : %s", &b.flameGeneral);
            elements[4] = new InterfaceElementCoreOption("Border (=)            : %s", &border);
            elements[5]
                = new InterfaceElementCoreOption("Translate (t,T)       : %s", &b.translate);
            elements[6] = new InterfaceElementCoreOption("Wave (w)              : %s", &b.wave);
            elements[7]
                = new InterfaceElementOption("Sound Processing (m,M): %s", &audioProcessing);
            elements[8] = new InterfaceElementCoreOption("Table (b,B)           : %s", &b.table);
            elements[9]
                = new InterfaceElementCoreOption("WaveScale (W)         : %s", &b.waveScale);
            elements[10] = new InterfaceElementCoreOption("Palette (p,P)         : %s", &b.palette);
            elements[11] = new InterfaceElementCoreOption("PCX (x,X))            : %s", &b.pcx);
            elements[12] = new InterfaceElementCoreOption("3D-Object (j,J)       : %s", &b.object);
            elements[13]
                = new InterfaceElementCoreOption("Flashlight (s)        : %s", &flashlight);
            elements[14] = new InterfaceElementCoreOption("Lights (y/Y)          : %s", &light);
            elements[15]
                = new InterfaceElementCoreOption("Background (i/I)      : %s", &background);
            elements[16] = new InterfaceElementCoreOption("Flies (c/C)           : %s", &fly);
        }
    }

    void preRun() {
        // update the Option pointers, might have changed because of switching to a different
        // buffer
        CthughaBuffer& b = CthughaBuffer::buffers[CthughaBuffer::nCurrent];
#define O(i)                                                                                       \
    ((InterfaceElementOption*)elements[i])->opt                                                    \
        = ((InterfaceElementCoreOption*)elements[i])->coreOpt
        O(2) = &b.flame;
        O(3) = &b.flameGeneral;
        O(4) = &border;
        O(5) = &b.translate;
        O(6) = &b.wave;
        O(8) = &b.table;
        O(9) = &b.waveScale;
        O(10) = &b.palette;
        O(11) = &b.pcx;
        O(12) = &b.object;
        O(13) = &flashlight;
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
    new InterfaceElementOption("Fire level               : %10s", &changeFireLevel, 10, 50, 100),
    new InterfaceElementOption("Little changes only      : %10s", &change_little),
    new InterfaceElementOption("Lock                     : %10s", &lock),
    new InterfaceElementOption("GL Mesh Size             : %10s", &MeshSize, 1, 10, 20),
    new InterfaceElementOption("GL Texture Quality       : %10s", &TextureQuality),
    new InterfaceElementOption("GL Hints                 : %10s", &Hints),
    new InterfaceElementOption("GL Dither                : %10s", &Dither),
    new InterfaceElementOption("GL Shade                 : %10s", &Shade),
};
int nElementsOption = sizeof(elementsOption) / sizeof(InterfaceElement*);

Interface interfaceOption("Options", "Options", NULL, elementsOption, nElementsOption);
