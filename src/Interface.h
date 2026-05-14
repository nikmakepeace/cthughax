// -*- C++ -*-

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "cthugha.h"
#include "CoreOption.h"
#include "keymap.h"

class InterfaceElement {
protected:
    const char * str;

public:
    InterfaceElement(const char * t) : str(t) {}
    virtual ~InterfaceElement() {}

    virtual const char * text(int /* selected */) { return str; }
    virtual int doKey(int /* key */) { return 1; }

    
};


class Interface {
public:
    static int saveToHot;
    static int showStatus;

    static Interface * current;
    static Interface * head;

    const char * name;
    const char * title;
    const char * text;

    InterfaceElement ** elements;
    int nElements;
    int sel;

    Interface * next;

    char * silenceMsg;
    int silenceLine;

    Interface(const char * n, const char * ti, const char * te);
    Interface(const char * n, const char * ti, const char * te,
	      InterfaceElement * el[], int nEl);

    virtual ~Interface();

    void setElements(InterfaceElement ** el, int nEl);
    static void set(const char * n);

    virtual void preRun() {}
    virtual void display();
    virtual void msg(char * msg);
    virtual void doKey(int key);
    virtual void run();

    
};


class InterfaceElementOption : public InterfaceElement {
public:
    Option * opt;
    int inc1;
    int inc2;
    int inc3;

    static Keymap keymap;

    InterfaceElementOption(const char * t, Option * o,
			   int i1 = 1, int i2 = 10, int i3 = 100);

    virtual const char * text(int selected);
    virtual int doKey(int key);
};


class InterfaceElementCoreOption : public InterfaceElementOption {
public:
    CoreOption * coreOpt;

    static Keymap coreKeymap;

    InterfaceElementCoreOption(const char * t, CoreOption * o,
			       int i1 = 1, int i2 = 10, int i3 = 100);

    virtual int doKey(int key);
};


class ErrorMessages {
    char msgs[128][128];
    int on_screen[128];
    int nMsgs;

public:
    ErrorMessages() : nMsgs(0) {}

    void addMessage(const char * text);
    void display();
};


extern Option * currentOption;
extern CoreOption * currentCoreOption;
extern InterfaceElementOption * currentOptionInterfaceElement;

extern Interface interfaceMixer;
extern ErrorMessages errors;

#endif
