#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "imath.h"
#include "Border.h"
#include "DisplayDevice.h"
#include "CthughaBuffer.h"
#include "Flashlight.h"

//
// ERROR:
//
// When the OptionList changes this Interface is not updated!
// this happends when new random palettes are generated
//

class InterfaceList : public Interface {
    static int size;
    int pos;
    CoreOption* coreOpt;
    static Keymap listOptionKeymap;

public:
    InterfaceList(const char* name, const char* title, CoreOption* o)
        : Interface(name, title, NULL)
        , coreOpt(o) { }

    virtual void display() {

        Interface::display();

        int n = coreOpt->getNEntries();
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
            sprintf(str1, "%c%s", (s == sel) ? '>' : ' ', coreOpt->entries[s]->name);

            char str2[128];
            sprintf(str2, "%s %3s%c", coreOpt->entries[s]->desc, coreOpt->entries[s]->use.text(),
                (s == sel) ? '<' : ' ');

            char str3[128];
            sprintf(str3, "%40s", str2);

            displayDevice->print(
                str3, (i + 2), 'l', (s == sel) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
            displayDevice->print(
                str1, (i + 2), 'l', (s == sel) ? TEXT_COLOR_HIGHLIGHT : TEXT_COLOR_NORMAL);
        }
    }

    virtual void doKey(int key) {
        int ret = key;
        int n = coreOpt->getNEntries();

        nElements = n;

        if ((sel < n) && (sel >= 0)) {
            currentOption = &(coreOpt->entries[sel]->use);
            currentCoreOption = coreOpt;

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
        currentCoreOption = NULL;
    }
};
Keymap InterfaceList::listOptionKeymap("ListOption");

ACTION(toggleUse) {
    if (currentOption == NULL)
        return;
    currentOption->change(+1);
}

ACTION(activate) {
    if (currentCoreOption == NULL)
        return;
    if (currentOption == NULL)
        return;

    currentCoreOption->entries[Interface::current->sel]->use.change("on");
    currentCoreOption->setValue(Interface::current->sel);
    currentCoreOption->change(0, 0);
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

InterfaceList interfaceList1("Flame", "Select Flame", &(CthughaBuffer::buffers[0].flame));

InterfaceList interfaceList2("Border", "Select Border of Buffer", &border);

InterfaceList interfaceList3(
    "Translate", "Select Translation Table", &(CthughaBuffer::buffers[0].translate));

InterfaceList interfaceList4("Wave", "Select Wave", &(CthughaBuffer::buffers[0].wave));

InterfaceList interfaceList6("Table", "Select Sound Table", &(CthughaBuffer::buffers[0].table));

InterfaceList interfaceList7(
    "WaveScaling", "Select Wave Scaling", &(CthughaBuffer::buffers[0].waveScale));

InterfaceList interfaceList8(
    "Object", "Select 3D Object (for some waves)", &(CthughaBuffer::buffers[0].object));

InterfaceList interfaceList9("Palette", "Select Palette", &(CthughaBuffer::buffers[0].palette));

InterfaceList interfaceListA("PCX", "Select PCX", &(CthughaBuffer::buffers[0].pcx));

InterfaceList interfaceListB("Flashlight", "Select Flashlight", &flashlight);
