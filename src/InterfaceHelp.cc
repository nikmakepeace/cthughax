#include "cthugha.h"
#include "Interface.h"
#include "InterfaceRuntime.h"
#include "display.h"
#include "keys.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"

class InterfaceHelp : public Interface {
    static const char* text[];
    static int nLines;
    double pos;
    int scrolling;

public:
    InterfaceHelp()
        : Interface("help", "Cthugha Help", NULL)
        , pos(0)
        , scrolling(0) { };
    virtual void display() {
        Interface::display();

        if (scrolling) {
            const double frameDelta = (cthughaDisplay != NULL)
                ? cthughaDisplay->currentFrameDeltaSeconds()
                : 0.0;
            pos = pos + frameDelta * 4.0;
        }

        for (int i = 0; i < (text_size.y - 2); i++) {
            int L = (int(pos) + i) % nLines;
            if (L < 0)
                continue;
            displayDevice->print(text[L] + 1, 3 - pos + int(pos) + i, 'l', text[L][0]);
        }
    }

    friend class toggleScrollingAction;
    friend class scrollUpAction;
    friend class scrollDownAction;

} interfaceHelp;

ACTION(toggleScrolling) { interfaceHelp.scrolling = !interfaceHelp.scrolling; }
ACTION(scrollUp) {
    interfaceHelp.pos -= 1;
    if (interfaceHelp.pos < 0)
        interfaceHelp.pos += interfaceHelp.nLines;
    interfaceHelp.scrolling = 0;
}
ACTION(scrollDown) {
    interfaceHelp.pos += 1;
    interfaceHelp.scrolling = 0;
}

#define N "\000"
#define H "\001"
#define E "\002"

const char* InterfaceHelp::text[] = {
    H "Help on Help                           \n",
    N "Down,+  : Scroll down 1 line           \n",
    N "Up,-    : Scroll up 1 line             \n",
    N "Space   : toggle automatic scrolling   \n",
    N "                                       \n",
    H "Function keys                          \n",
    N "F1,?    : Help                         \n",
    N "F2, o   : Effect Controls Summary      \n",
    N "F3, O   : Time/Auto-changer Options    \n",
    N "F4      : toggle FPS counter           \n",
    N "F5      : Sound control panel          \n",
    N "F6      : Mixer                        \n",
    N "F7/F8   : Play List (not implemented)  \n",
    N "                                       \n",
    N "F9      : select display               \n",
    N "F10     : select flame                 \n",
    N "F11     : select buffer border         \n",
    N "F12     : select translation table     \n",
    N "F13     : select wave                  \n",
    N "F14     : select sound processing      \n",
    N "F15     : select table                 \n",
    N "F16     : select wave scaling          \n",
    N "F17     : select wave object           \n" N "F18     : select palette               \n",
    N "F19     : select image                 \n",
    N "F20     : select flashlight            \n",
    N "                                       \n",
    N "                                       \n",
    H "Change keys                            \n",
    N "l,L     : toggle lock (automatic changing on/off)\n",
    N "                                       \n",
    N "w       : next wave                    \n",
    N "W       : next wave scaling factor     \n",
    N "f/F     : next/prev flame              \n",
    N "g/G     : change general flame table   \n",
    N "d/D     : next/prev display            \n",
    N "p/P     : next/prev palette            \n",
    N "r       : re-roll current random map   \n",
    N "R       : add saved random palette     \n",
    N "t/T     : next/prev translate          \n",
    N "x/X     : show current image and change\n",
    N "j/J     : next/prev object (used by some waves)\n",
    N "b/B     : next/prev table (used by some waves)\n",
    N "m/M     : next/prev sound processing   \n",
    N "=       : next buffer border           \n",
    N "s/S     : toggle flashlight            \n",
    N "                                       \n",
    N "z/Z     : change zoom value            \n",
    N "./,     : toggle status display        \n",
    N "                                       \n",
    N "Space   : random change of all options \n",
    N "Enter   : random change of one option  \n",
    N "Backspace : undo last change           \n",
    N "                                       \n",
    H "Preset slots                           \n",
    N "h+0, h+1, ..., h+9  or                 \n",
    N "Shift+0 ... Shift+9                    \n",
    N "          save Effect Controls to preset\n",
    N "0, 1, ... 9 : restore preset setup     \n",
    N "                                       \n",
    H "Other keys                             \n",
    N "n/N     : update sound device          \n",
    N "a/A     : save current setup to        \n",
    N "          ~/.cthugha.auto              \n",
    N "                                       \n",
    N "q/ESC: Exit                            \n",
    N "                                       \n",
    N "                                       \n",
    H "Changing Options                       \n",
    N "Up/Down : Move selection               \n",
    N "Left/Right  -/+  //*  :                \n",
    N "          Increment/Decrement          \n",
    N "0, 1, ... 9 : set value                \n",
    N "                                       \n",
    N "                                       \n",
};
int InterfaceHelp::nLines = sizeof(InterfaceHelp::text) / sizeof(const char*);

void registerHelpInterface(InterfaceRuntime& runtime) {
    runtime.registerInterface(interfaceHelp);
}
