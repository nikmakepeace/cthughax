// -*- C++ -*-

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "cthugha.h"
#include "EffectControl.h"

class AutoChangeControls;
class AudioProcessingSelector;
class CommandContext;
class CommandDispatcher;
class CommandRegistry;
class DisplayPresentationSettings;
class InputQueue;
class InterfaceRuntime;
class KeymapRegistry;
class Option;
class OverlayRenderContext;
class RuntimeConfigRegistry;

class InterfaceElement {
protected:
    const char* str;

public:
    InterfaceElement(const char* t)
        : str(t) { }
    virtual ~InterfaceElement() { }

    virtual const char* text(InterfaceRuntime& /* runtime */,
        const OverlayRenderContext& /* overlay */, int /* selected */) {
        return str;
    }
    virtual int doKey(InterfaceRuntime& /* runtime */,
        KeymapRegistry& /* keymaps */, CommandRegistry& /* commands */,
        CommandDispatcher& /* dispatcher */, CommandContext& /* context */,
        int /* key */) { return 1; }
};

class Interface {
public:
    const char* name;
    const char* title;
    const char* text;

    InterfaceElement** elements;
    int nElements;
    int sel;

    Interface(const char* n, const char* ti, const char* te);
    Interface(const char* n, const char* ti, const char* te, InterfaceElement* el[], int nEl);

    virtual ~Interface();

    void setElements(InterfaceElement** el, int nEl);

    virtual void preRun() { }
    virtual void display(InterfaceRuntime& runtime,
        OverlayRenderContext& overlay);
    virtual void doKey(InterfaceRuntime& runtime, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context, int key);

    /**
     * Services the active interface once.
     *
     * Application runs the interface before and after frame generation so input
     * handling and overlay/status updates can bracket visual work.
     *
     * @param runtime Runtime state owning the current interface selection and
     *        adapter pointers.
     */
    virtual void run(InterfaceRuntime& runtime, InputQueue& inputQueue,
        KeymapRegistry& keymaps, CommandRegistry& commands,
        CommandDispatcher& dispatcher, CommandContext& context);
};

class InterfaceElementOption : public InterfaceElement {
public:
    Option* opt;
    int inc1;
    int inc2;
    int inc3;

    InterfaceElementOption(const char* t, Option* o, int i1 = 1, int i2 = 10, int i3 = 100);

    virtual const char* text(InterfaceRuntime& runtime,
        const OverlayRenderContext& overlay, int selected);
    virtual int doKey(InterfaceRuntime& runtime, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context, int key);
};

class InterfaceElementEffectControl : public InterfaceElementOption {
public:
    EffectControl* effectControl;

    InterfaceElementEffectControl(const char* t, EffectControl* o, int i1 = 1, int i2 = 10, int i3 = 100);

    virtual int doKey(InterfaceRuntime& runtime, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context, int key);
};

class ErrorMessages {
    char msgs[128][128];
    int on_screen[128];
    int nMsgs;

public:
    ErrorMessages()
        : nMsgs(0) { }

    void addMessage(const char* text, InterfaceRuntime& runtime);
    void display(InterfaceRuntime& runtime, OverlayRenderContext& overlay);
};

/**
 * Registers the existing concrete panel definitions into an owned runtime.
 *
 * @param runtime Interface runtime that will own selection/adapter state.
 */
void registerDefaultInterfaces(InterfaceRuntime& runtime,
    Option& quietMessageOption, DisplayPresentationSettings& displaySettings);
void registerInterfaceKeyActions(CommandRegistry& registry);

#endif
