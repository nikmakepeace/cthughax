// -*- C++ -*-

#ifndef __KEYMAP_H
#define __KEYMAP_H

#include "cthugha.h"

struct InputConfig;
class AudioProcessingState;
class RuntimeCommandSink;

class Action {
    const char* name;
    int nameLen;

    static Action* head;
    Action* next;

public:
    Action(const char* n)
        : name(n)
        , next(head) {
        head = this;
        nameLen = strlen(name);
    }
    virtual ~Action() { }
    virtual void act(const char* /* param */, double /* value */) { };

    int operator==(const char* n) const { return (strncasecmp(name, n, nameLen) == 0); }

    friend class Keymap;
};

#define ACTION(a)                                                                                  \
    class a##Action : public Action {                                                              \
    public:                                                                                        \
        a##Action()                                                                                \
            : Action(#a) { }                                                                       \
        virtual void act(const char* param, double value);                                         \
    } a##ActionImpl;                                                                               \
    void a##Action::act(const char* p, double v)

class Keymap {
protected:
    static Keymap* first;
    Keymap* next;

    static Keymap* current;

    const char* name;
    struct Binding {
        int key;
        struct ActionList {
            Action* action;
            const char* param;
            ActionList* next;

            ActionList(Action* a, const char* p, ActionList* n)
                : action(a)
                , param(p)
                , next(n) { }
        }* actionList;

        Binding()
            : key(0)
            , actionList(NULL) { }
    };
    struct BindingList : Binding {
        BindingList* next;

        BindingList(Binding& b, BindingList* n)
            : Binding(b)
            , next(n) { }
    }* bindingList;

    static Keymap* find(const char* name, int create = 0);

    void add(Binding& b);
    Binding parseBinding(const char* line);

public:
    Keymap(const char* name);

    static void readFile(const char* fileName);
    static void addList(int dummy, ...);

    void add(const char* line);

    int action(int key);
    static int action(const char* name, int key);

    static void set(const char* name);
    static void init(const InputConfig& config);
    static void setRuntimeCommandSink(RuntimeCommandSink* sink);
    static RuntimeCommandSink* runtimeCommandSink();

    /**
     * Installs the audio-processing state used for continuation saves.
     *
     * @param state State to read; NULL omits audio processing from continuation.
     */
    static void setAudioProcessingState(const AudioProcessingState* state);

    /**
     * Returns the audio-processing state used for continuation saves.
     *
     * @return Installed state, or NULL before runtime setup.
     */
    static const AudioProcessingState* audioProcessingState();
};

#endif
