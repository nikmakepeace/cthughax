#include "cthugha.h"
#include "DisplayDevice.h"
#include "cthugha.h"
#include "display.h"
#include "disp-sys.h"
#include "imath.h"
#include "xcthugha.h"
#include "Sound.h"
#include "keys.h"
#include "CthughaBuffer.h"
#include "Interface.h"
#include "cth_buffer.h"
#include "CthughaDisplay.h"

#include <unistd.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if WITH_SAVER == 1
#include <X11/extensions/scrnsaver.h>
#endif
#include <X11/extensions/XShm.h>

#include "xcthugha.h"

//
// Create the panel with buttons and menus
//
void DisplayDeviceX11::key_button(Widget /*w*/, XtPointer data, XtPointer /*data2*/) {
    if (data)
        keys_x11((char*)data);
}
/*
 * handler for activated menu-items
 */
void DisplayDeviceX11::menuCB(Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    menu_data_t* d = (menu_data_t*)data;

    if (d->opt != 0) {
        d->opt->setValue(d->pos);
        d->opt->change(0, 0);
    }
}

Widget DisplayDeviceX11::add_menu(
    char* name, CoreOption* what, Widget parent, Widget under, Widget right) {
    Arg wargs[3];
    int n;
    Widget button, menu, item;
    int i;

    if (parent == NULL)
        return NULL;

    /* create menu button */
    n = 0;
    XtSetArg(wargs[n], XtNlabel, name);
    n++;
    if (under) {
        XtSetArg(wargs[n], XtNfromVert, under);
        n++;
    }
    if (right) {
        XtSetArg(wargs[n], XtNfromHoriz, right);
        n++;
    }
    button = XtCreateManagedWidget(name, menuButtonWidgetClass, parent, wargs, n);
    if (button == NULL)
        return NULL;

    /* create the menu popup */
    menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, button, NULL, 0);
    if (menu == NULL) {
        XtDestroyWidget(button);
        return NULL;
    }

    if (what->getNEntries() <= 0) {
        XtSetArg(wargs[0], XtNlabel, "none");
        XtCreateManagedWidget("none", smeBSBObjectClass, menu, wargs, 1);
    }

    /* create menu items */
    for (i = 0; i < what->getNEntries(); i++) {
        menu_data_t* md = new menu_data_t;

        md->opt = what;
        md->pos = i;

        XtSetArg(wargs[0], XtNlabel, (*what)[i]->Name());
        item = XtCreateManagedWidget((*what)[i]->Name(), smeBSBObjectClass, menu, wargs, 1);
        XtAddCallback(item, XtNcallback, menuCB, md);
    }
    return button;
}
void DisplayDeviceX11::xcth_create_panel() {
    Widget panel;
    Widget quit_button, change_button;
    Widget menu[7];
    Arg wargs[3];

    /* create the panel formWidget */
    panel = XtCreateManagedWidget("panel", formWidgetClass, xcth_toplevel, NULL, 0);

    /* create the quit button */
    XtSetArg(wargs[0], XtNlabel, "Quit!");
    quit_button = XtCreateManagedWidget("quit", commandWidgetClass, panel, wargs, 1);
    XtAddCallback(quit_button, XtNcallback, (XtCallbackProc)quit, NULL);

    /* create the change button */
    XtSetArg(wargs[0], XtNlabel, "Change!");
    XtSetArg(wargs[1], XtNfromHoriz, quit_button);
    change_button = XtCreateManagedWidget("change", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(change_button, XtNcallback, key_button, (char*)" ");

    /* create the menus */
    menu[0] = add_menu("Display", &::screen, panel, quit_button, NULL);
    menu[1] = add_menu("Wave", &(CthughaBuffer::current->wave), panel, quit_button, menu[0]);
    menu[2] = add_menu("Flame", &CthughaBuffer::current->flame, panel, quit_button, menu[1]);
    menu[3]
        = add_menu("Translation", &CthughaBuffer::current->translate, panel, quit_button, menu[2]);
    menu[4] = add_menu("Palette", &CthughaBuffer::current->palette, panel, quit_button, menu[3]);
    menu[5] = add_menu("PCX", &CthughaBuffer::current->pcx, panel, quit_button, menu[4]);
    menu[6] = add_menu("Objects", &CthughaBuffer::current->object, panel, quit_button, menu[5]);

    // create the panelText Widget
    text_size.x = 80;
    text_size.y = 25;
    XtSetArg(wargs[0], XtNfromVert, menu[0]);
    XtSetArg(wargs[1], XtNwidth, fontSize.x * text_size.x);
    XtSetArg(wargs[2], XtNheight, fontSize.y * text_size.y);
    panelTextWidget = XtCreateManagedWidget("panelText", labelWidgetClass, panel, wargs, 3);

    /* realize window */
    XtRealizeWidget(xcth_toplevel);
}
