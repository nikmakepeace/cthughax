#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "imath.h"
#include "Border.h"
#include "DisplayDevice.h"
#include "CthughaBuffer.h"
#include "Flashlight.h"
#include "RuntimeCommandSink.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "flames.h"
#include "TranslationOptions.h"
#include "waves.h"

//
// ERROR:
//
// When the OptionList changes this Interface is not updated!
// this happends when new random palettes are generated
//

class InterfaceList : public Interface {
    static int size;
    int pos;
    EffectControl* effectControl;
    static Keymap listOptionKeymap;

public:
    InterfaceList(const char* name, const char* title, EffectControl* o)
        : Interface(name, title, NULL)
        , effectControl(o) { }

    virtual void display() {

        Interface::display();

        int n = effectControl->getNEntries();
        if (n == 0)
            return;

        // make sure selected line is on screen
        if (sel < pos) {
            pos = max(sel, 0);
        }
        if ((sel - size) >= pos) {
            pos = sel;
        }
        // display size lines
        for (int i = 0; i < size; i++) {
            int s = i + pos;
            if ((s >= n) || (s < 0))
                continue;

            char str1[128];
            snprintf(str1, sizeof(str1), "%c%s", (s == sel) ? '>' : ' ', effectControl->entries[s]->name);

            char str2[128];
            snprintf(str2, sizeof(str2), "%s %3s%c", effectControl->entries[s]->desc, effectControl->entries[s]->use.text(),
                (s == sel) ? '<' : ' ');

            char str3[128];
            snprintf(str3, sizeof(str3), "%40s", str2);

            displayDevice->print(
                str3, (i + 2), 'l', (s == sel) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
            displayDevice->print(
                str1, (i + 2), 'l', (s == sel) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
        }
    }

    virtual void doKey(int key) {
        int ret = key;
        int n = effectControl->getNEntries();

        nElements = n;

        if ((sel < n) && (sel >= 0)) {
            currentOption = &(effectControl->entries[sel]->use);
            currentEffectControl = effectControl;

            ret = Keymap::action("ListOption", key);

            if (ret)
                ret = Keymap::action("Option", key);
        }

        if (ret)
            ret = Keymap::action(name, key);
        if (ret)
            ret = Keymap::action("list", key);
        if (ret)
            ret = Keymap::action("default", key);

        nElements = 0;

        currentOption = NULL;
        currentEffectControl = NULL;
    }
};
Keymap InterfaceList::listOptionKeymap("ListOption");

ACTION(toggleUse) {
    if (currentOption == NULL)
        return;
    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (sink != NULL)
        sink->apply(RuntimeCommand::changeOptionBy(*currentOption, +1));
}

ACTION(activate) {
    if (currentEffectControl == NULL)
        return;
    if (currentOption == NULL)
        return;

    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (sink != NULL)
        sink->apply(RuntimeCommand::activateEffectControl(
            *currentEffectControl, Interface::current->sel));
}

#if 0
int InterfaceList::do_key(int key) {
    switch(key) {
    case CK_ENTER:
	if(sel >= nFixed) {
	    optList->entries[sel-nFixed]->use.setValue(1);	// make it usable
	    optList->setValue(sel - nFixed);			// make it active
	    optList->change(0, 0);				// validate and action
	    set(interfaces[0]);					// and close this display
	    return 0;
	} else
	    return key;
    case ' ':
	if(sel >= nFixed) {
	    optList->entries[sel-nFixed]->use.setValue(1);	// make it usable
	    optList->setValue(sel - nFixed);			// make it active
	    optList->change(0, 0);				// validate and action
	    return 0;
	} else
	    return key;
    }
    return Interface::do_key(key);
}
#endif

int InterfaceList::size = 6;

InterfaceList interfaceList0("Display", "Select Display", &screen);

InterfaceList interfaceList1("Flame", "Select Flame", &flame);

InterfaceList interfaceList2("Border", "Select Border of Buffer", &border);

InterfaceList interfaceList3("Translate", "Select Translation Table", &translation);

InterfaceList interfaceList4("Wave", "Select Wave", &wave);

InterfaceList interfaceList6("Table", "Select Sound Table", &table);

InterfaceList interfaceList7("WaveScaling", "Select Wave Scaling", &waveScale);

InterfaceList interfaceList8("Object", "Select 3D Object (for some waves)", &object);

InterfaceList interfaceList9("Palette", "Select Palette", &palette);

InterfaceList interfaceListA("Image", "Select Image", &(videoDirector().imageOption()));

InterfaceList interfaceListB("Flashlight", "Select Flashlight", &flashlight);
