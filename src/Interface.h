// -*- C++ -*-

#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "cthugha.h"
#include "EffectControl.h"
#include "keymap.h"

class AutoChangerStatusProvider;
class AutoChangeControls;
class AudioProcessingSelector;
class RuntimeConfigRegistry;

class InterfaceElement {
protected:
    const char* str;

public:
    InterfaceElement(const char* t)
        : str(t) { }
    virtual ~InterfaceElement() { }

    virtual const char* text(int /* selected */) { return str; }
    virtual int doKey(int /* key */) { return 1; }
};

class Interface {
    static RuntimeConfigRegistry* runtimeConfigRegistryValue;
    static const AutoChangerStatusProvider* autoChangerStatusProviderValue;
    static AutoChangeControls* autoChangeControlsValue;
    static AudioProcessingSelector* audioProcessingSelectorValue;

public:
    static int saveToPreset;
    static int showStatus;

    static Interface* current;
    static Interface* head;

    const char* name;
    const char* title;
    const char* text;

    InterfaceElement** elements;
    int nElements;
    int sel;

    Interface* next;

    Interface(const char* n, const char* ti, const char* te);
    Interface(const char* n, const char* ti, const char* te, InterfaceElement* el[], int nEl);

    virtual ~Interface();

    void setElements(InterfaceElement** el, int nEl);

    /**
     * Installs the registry used for displaying current runtime selections.
     *
     * @param registry Registry to read; NULL restores direct option fallbacks.
     */
    static void setRuntimeConfigRegistry(RuntimeConfigRegistry* registry);

    /**
     * Returns the registry used for displaying current runtime selections.
     *
     * @return Installed registry, or NULL when not yet available.
     */
    static const RuntimeConfigRegistry* runtimeConfigRegistry();

    /**
     * Installs the provider used for automatic scene-change status text.
     *
     * @param provider Provider to read; NULL hides auto-change status.
     */
    static void setAutoChangerStatusProvider(
        const AutoChangerStatusProvider* provider);

    /**
     * Returns the provider used for automatic scene-change status text.
     *
     * @return Installed provider, or NULL when AutoChanger is unavailable.
     */
    static const AutoChangerStatusProvider* autoChangerStatusProvider();

    /**
     * Installs Option adapters for automatic scene-change panel controls.
     *
     * @param controls Controls to read; NULL disables those panel rows.
     */
    static void setAutoChangeControls(AutoChangeControls* controls);

    /**
     * Returns Option adapters for automatic scene-change panel controls.
     *
     * @return Installed controls, or NULL before runtime setup.
     */
    static AutoChangeControls* autoChangeControls();

    /**
     * Installs the selector used for the audio-processing option panel row.
     *
     * @param selector Selector to read and mutate; NULL disables the row.
     */
    static void setAudioProcessingSelector(AudioProcessingSelector* selector);

    /**
     * Returns the selector used for the audio-processing option panel row.
     *
     * @return Installed selector, or NULL before runtime setup.
     */
    static AudioProcessingSelector* audioProcessingSelector();

    static void set(const char* n);

    virtual void preRun() { }
    virtual void display();
    virtual void doKey(int key);

    /**
     * Services the active interface once.
     *
     * Application runs the interface before and after frame generation so input
     * handling and overlay/status updates can bracket visual work.
     */
    virtual void run();
};

class InterfaceElementOption : public InterfaceElement {
public:
    Option* opt;
    int inc1;
    int inc2;
    int inc3;

    static Keymap keymap;

    InterfaceElementOption(const char* t, Option* o, int i1 = 1, int i2 = 10, int i3 = 100);

    virtual const char* text(int selected);
    virtual int doKey(int key);
};

class InterfaceElementEffectControl : public InterfaceElementOption {
public:
    EffectControl* effectControl;

    static Keymap effectControlKeymap;

    InterfaceElementEffectControl(const char* t, EffectControl* o, int i1 = 1, int i2 = 10, int i3 = 100);

    virtual int doKey(int key);
};

class ErrorMessages {
    char msgs[128][128];
    int on_screen[128];
    int nMsgs;

public:
    ErrorMessages()
        : nMsgs(0) { }

    void addMessage(const char* text);
    void display();
};

extern Option* currentOption;
extern EffectControl* currentEffectControl;
extern InterfaceElementOption* currentOptionInterfaceElement;

extern Interface interfaceMixer;
extern ErrorMessages errors;

#endif
