/** @file
 * Unit coverage for the Commands/Input module root.
 */

#include "CommandsInputRuntime.h"

#include "Configuration.h"
#include "DisplayPresentationOptions.h"
#include "InterfaceRuntime.h"
#include "Option.h"
#include "ProcessServices.h"
#include "keys.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeMillisecondClock : public MillisecondClock {
    int value;

public:
    explicit FakeMillisecondClock(int value_)
        : value(value_) { }

    virtual int milliseconds() const { return value; }
};

class TrackingInterface : public Interface {
public:
    int runCount;
    InterfaceRuntime* runtimeSeen;
    InputQueue* inputQueueSeen;
    KeymapRegistry* keymapsSeen;
    CommandRegistry* commandsSeen;
    CommandDispatcher* dispatcherSeen;

    TrackingInterface()
        : Interface("tracking", "Tracking", NULL)
        , runCount(0)
        , runtimeSeen(NULL)
        , inputQueueSeen(NULL)
        , keymapsSeen(NULL)
        , commandsSeen(NULL)
        , dispatcherSeen(NULL) { }

    virtual void run(InterfaceRuntime& runtime, InputQueue& inputQueue,
        KeymapRegistry& keymaps, CommandRegistry& commands,
        CommandDispatcher& dispatcher, CommandContext&) {
        ++runCount;
        runtimeSeen = &runtime;
        inputQueueSeen = &inputQueue;
        keymapsSeen = &keymaps;
        commandsSeen = &commands;
        dispatcherSeen = &dispatcher;
    }
};

static int defaultInterfaceRegistrationCount = 0;
static int defaultKeyRegistrationCount = 0;
static int interfaceKeyRegistrationCount = 0;
static int keymapInitCount = 0;
static int keymapInitEscapeEnabled = 0;

void registerDefaultInterfaces(InterfaceRuntime& runtime,
    Option&, DisplayPresentationSettings&) {
    ++defaultInterfaceRegistrationCount;
    runtime.registerOwnedInterface(new Interface("main", "Main", NULL));
}

void registerDefaultKeyActions(CommandRegistry&) {
    ++defaultKeyRegistrationCount;
}

void registerInterfaceKeyActions(CommandRegistry&) {
    ++interfaceKeyRegistrationCount;
}

Interface::Interface(const char* n, const char* ti, const char* te)
    : name(n)
    , title(ti)
    , text(te)
    , elements(NULL)
    , nElements(0)
    , sel(-1) { }

Interface::Interface(const char* n, const char* ti, const char* te,
    InterfaceElement** el, int nEl)
    : name(n)
    , title(ti)
    , text(te)
    , elements(el)
    , nElements(nEl)
    , sel(-1) { }

Interface::~Interface() { }

void Interface::setElements(InterfaceElement** el, int nEl) {
    elements = el;
    nElements = nEl;
}

void Interface::display(InterfaceRuntime&, OverlayRenderContext&) { }

void Interface::doKey(InterfaceRuntime&, KeymapRegistry&, CommandRegistry&,
    CommandDispatcher&, CommandContext&, int) { }

void Interface::run(InterfaceRuntime&, InputQueue&, KeymapRegistry&,
    CommandRegistry&, CommandDispatcher&, CommandContext&) { }

CommandRegistry::CommandRegistry()
    : firstValue(NULL) { }

CommandRegistry::~CommandRegistry() { }

void CommandRegistry::registerAction(Action*) { }

Action* CommandRegistry::find(const char*) const { return NULL; }

Action* CommandRegistry::findLongestPrefix(const char*) const { return NULL; }

KeymapRegistry::KeymapRegistry()
    : firstValue(NULL) { }

KeymapRegistry::~KeymapRegistry() { }

void KeymapRegistry::init(const InputConfig& config, CommandRegistry&) {
    ++keymapInitCount;
    keymapInitEscapeEnabled = config.escapeKeyEnabled;
}

CommandContext::CommandContext(InterfaceRuntime& runtime,
    RuntimeCommandSink* runtimeCommandSink,
    RuntimeCommandTargetRouter* commandRouter)
    : runtimeValue(&runtime)
    , runtimeCommandSinkValue(runtimeCommandSink)
    , commandRouterValue(commandRouter)
    , optionValue(NULL)
    , effectControlValue(NULL)
    , sceneTargetValue(RuntimeSceneFlame)
    , hasSceneTargetValue(0)
    , optionElementValue(NULL)
    , effectChoiceIndexValue(-1) { }

InterfaceRuntime& CommandContext::runtime() const {
    return *runtimeValue;
}

RuntimeCommandSink* CommandContext::runtimeCommandSink() const {
    return runtimeCommandSinkValue;
}

RuntimeCommandTargetRouter* CommandContext::commandRouter() const {
    return commandRouterValue;
}

void CommandContext::targetOption(Option&, InterfaceElementOption&) { }
void CommandContext::targetEffectControl(EffectControl&,
    InterfaceElementOption&) { }
void CommandContext::targetEffectChoice(EffectControl&, Option&, int) { }
void CommandContext::targetSceneSelection(RuntimeSceneTarget,
    InterfaceElementOption&) { }
void CommandContext::targetSceneChoice(RuntimeSceneTarget, int) { }
void CommandContext::changeValueByElementIncrement(int, double) { }
void CommandContext::setValueFromElement(double) { }
void CommandContext::toggleEffectControlLock() { }
void CommandContext::toggleEffectChoiceUse() { }
void CommandContext::activateEffectChoice() { }

int CommandDispatcher::dispatch(const std::vector<ActionInvocation>&,
    CommandContext&) const {
    return 1;
}

int CommandDispatcher::dispatchKeymap(KeymapRegistry&, CommandRegistry&,
    const char*, int, CommandContext&) const {
    return 1;
}

static void resetCounters() {
    defaultInterfaceRegistrationCount = 0;
    defaultKeyRegistrationCount = 0;
    interfaceKeyRegistrationCount = 0;
    keymapInitCount = 0;
    keymapInitEscapeEnabled = 0;
}

static void testConstructorRegistersDefaultInterfaces() {
    resetCounters();
    FakeMillisecondClock clock(1000);
    OptionTime quietMessageOption("quiet-message", 0);
    DisplayPresentationSettings displaySettings;

    CommandsInputRuntime runtime(clock, quietMessageOption, displaySettings);

    assert(defaultInterfaceRegistrationCount == 1);
    assert(runtime.interfaceRuntime().find("main") != NULL);
    assert(runtime.interfaceRuntime().milliseconds() == 1000);
}

static void testConfigureInputDelegatesToOwnedQueue() {
    resetCounters();
    FakeMillisecondClock clock(1000);
    OptionTime quietMessageOption("quiet-message", 0);
    DisplayPresentationSettings displaySettings;
    CommandsInputRuntime runtime(clock, quietMessageOption, displaySettings);
    char escapeText[2] = { 27, 0 };

    runtime.inputQueue().pushRawKey(escapeText, 0);
    assert(runtime.inputQueue().popKey() == CK_NONE);

    InputConfig config;
    config.escapeKeyEnabled = 1;
    runtime.configureInput(config);
    runtime.inputQueue().pushRawKey(escapeText, 0);

    assert(runtime.inputQueue().popKey() == CK_ESC);
    assert(runtime.inputQueue().popKey() == CK_NONE);
}

static void testRegistersAndInitializesOwnedCommandState() {
    resetCounters();
    FakeMillisecondClock clock(1000);
    OptionTime quietMessageOption("quiet-message", 0);
    DisplayPresentationSettings displaySettings;
    CommandsInputRuntime runtime(clock, quietMessageOption, displaySettings);

    runtime.registerBuiltins();

    assert(defaultKeyRegistrationCount == 1);
    assert(interfaceKeyRegistrationCount == 1);

    InputConfig config;
    config.escapeKeyEnabled = 1;
    runtime.initializeKeymaps(config);

    assert(keymapInitCount == 1);
    assert(keymapInitEscapeEnabled == 1);
}

static void testRunCurrentUsesOnlyOwnedState() {
    resetCounters();
    FakeMillisecondClock clock(1000);
    OptionTime quietMessageOption("quiet-message", 0);
    DisplayPresentationSettings displaySettings;
    CommandsInputRuntime runtime(clock, quietMessageOption, displaySettings);
    TrackingInterface trackingInterface;
    CommandContext context(runtime.interfaceRuntime(), NULL, NULL);

    runtime.interfaceRuntime().registerInterface(trackingInterface);
    runtime.interfaceRuntime().set("tracking");
    runtime.runCurrent(context);

    assert(trackingInterface.runCount == 1);
    assert(trackingInterface.runtimeSeen == &runtime.interfaceRuntime());
    assert(trackingInterface.inputQueueSeen == &runtime.inputQueue());
    assert(trackingInterface.keymapsSeen == &runtime.keymaps());
    assert(trackingInterface.commandsSeen == &runtime.commands());
    assert(trackingInterface.dispatcherSeen == &runtime.dispatcher());
}

int main() {
    testConstructorRegistersDefaultInterfaces();
    testConfigureInputDelegatesToOwnedQueue();
    testRegistersAndInitializesOwnedCommandState();
    testRunCurrentUsesOnlyOwnedState();
    return 0;
}
