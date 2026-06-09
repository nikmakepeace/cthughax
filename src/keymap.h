// -*- C++ -*-

#ifndef __KEYMAP_H
#define __KEYMAP_H

#include "cthugha.h"
#include "RuntimeCommand.h"

#include <vector>

struct InputConfig;
class EffectControl;
class InterfaceElementOption;
class InterfaceRuntime;
class Option;
class RuntimeCommandSink;
class RuntimeCommandTargetRouter;

struct ActionInvocation;
class CommandContext;

class Action {
    const char* name;
    int nameLen;

    Action* next;

public:
    Action(const char* n)
        : name(n)
        , nameLen(strlen(n))
        , next(NULL) {
    }
    virtual ~Action() { }
    virtual void act(const ActionInvocation& /* invocation */,
        CommandContext& /* context */) { };

    int operator==(const char* n) const { return (strncasecmp(name, n, nameLen) == 0); }
    int length() const { return nameLen; }

    friend class CommandRegistry;
    friend class Keymap;
};

struct ActionInvocation {
    Action* target;
    const char* name;
    const char* param;
    double value;

    ActionInvocation(Action* target_, const char* name_,
        const char* param_, double value_)
        : target(target_)
        , name(name_)
        , param(param_)
        , value(value_) { }
};

class CommandContext {
    InterfaceRuntime* runtimeValue;
    RuntimeCommandSink* runtimeCommandSinkValue;
    RuntimeCommandTargetRouter* commandRouterValue;
    Option* optionValue;
    EffectControl* effectControlValue;
    RuntimeSceneTarget sceneTargetValue;
    int hasSceneTargetValue;
    InterfaceElementOption* optionElementValue;
    int effectChoiceIndexValue;

public:
    CommandContext(InterfaceRuntime& runtime,
        RuntimeCommandSink* runtimeCommandSink,
        RuntimeCommandTargetRouter* commandRouter);

    InterfaceRuntime& runtime() const;
    RuntimeCommandSink* runtimeCommandSink() const;
    RuntimeCommandTargetRouter* commandRouter() const;

    void targetOption(Option& option, InterfaceElementOption& element);
    void targetEffectControl(EffectControl& effectControl,
        InterfaceElementOption& element);
    void targetEffectChoice(EffectControl& effectControl, Option& option,
        int selectedIndex);

    /**
     * Targets a typed Scene visual selection for scoped row-edit actions.
     *
     * @param sceneTarget Scene selection to mutate.
     * @param element Interface element supplying increment metadata.
     */
    void targetSceneSelection(RuntimeSceneTarget sceneTarget,
        InterfaceElementOption& element);

    /**
     * Targets a typed Scene visual choice for scoped list actions.
     *
     * @param sceneTarget Scene selection containing the choice.
     * @param selectedIndex Choice index under the cursor.
     */
    void targetSceneChoice(RuntimeSceneTarget sceneTarget, int selectedIndex);

    void changeValueByElementIncrement(int incrementIndex, double value);
    void setValueFromElement(double value);
    void toggleEffectControlLock();
    void toggleEffectChoiceUse();
    void activateEffectChoice();
};

class CommandRegistry;
class KeymapRegistry;

class CommandDispatcher {
public:
    int dispatch(const std::vector<ActionInvocation>& invocations,
        CommandContext& context) const;
    int dispatchKeymap(KeymapRegistry& keymaps, CommandRegistry& commands,
        const char* keymapName, int key, CommandContext& context) const;
};

#define ACTION(a)                                                                                  \
    class a##Action : public Action {                                                              \
    public:                                                                                        \
        a##Action()                                                                                \
            : Action(#a) { }                                                                       \
        virtual void act(const ActionInvocation& invocation, CommandContext& context);             \
    };                                                                                             \
    void a##Action::act(const ActionInvocation& invocation, CommandContext& context)

class CommandRegistry {
    Action* firstValue;

public:
    CommandRegistry();
    ~CommandRegistry();

    void registerAction(Action* action);
    Action* find(const char* name) const;
    Action* findLongestPrefix(const char* text) const;
};

class Keymap {
protected:
    Keymap* next;

    char* name;
    struct Binding {
        int key;
        struct ActionList {
            char* actionName;
            char* param;
            ActionList* next;

            ActionList(char* a, char* p, ActionList* n)
                : actionName(a)
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

    void add(Binding& b);
    Binding parseBinding(const char* line, CommandRegistry& commands);

public:
    Keymap(const char* name);
    ~Keymap();

    void add(const char* line, CommandRegistry& commands);

    int actionsFor(int key, CommandRegistry& commands,
        std::vector<ActionInvocation>& invocations) const;

    friend class KeymapRegistry;
};

class KeymapRegistry {
    Keymap* firstValue;

    Keymap* find(const char* name, int create = 0);

public:
    KeymapRegistry();
    ~KeymapRegistry();

    void readFile(const char* fileName, CommandRegistry& commands);
    void addList(CommandRegistry& commands, int dummy, ...);

    int actionsFor(const char* name, int key, CommandRegistry& commands,
        std::vector<ActionInvocation>& invocations) const;

    void init(const InputConfig& config, CommandRegistry& commands);

};

void registerDefaultKeyActions(CommandRegistry& registry);

#endif
