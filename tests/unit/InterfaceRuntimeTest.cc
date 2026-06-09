/** @file
 * Unit coverage for owned InterfaceRuntime state and scoped command context.
 */

#include "cthugha.h"
#include "InterfaceRuntime.h"
#include "Interface.h"
#include "InputQueue.h"
#include "Option.h"
#include "ProcessServices.h"
#include "RuntimeCommandSink.h"
#include "RuntimeCommandTargets.h"
#include "SceneVisualSelections.h"
#include "keymap.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

Option::~Option() { }

class SimpleOption : public Option {
public:
    SimpleOption()
        : Option("simple") { }

    virtual void change(int) { }
    virtual void change(const char*) { }
    virtual const char* text() const { return "simple"; }
};

class FakeSceneSelection : public SceneFlameSelection,
    public SceneGeneralFlameSelection,
    public SceneWaveSelection,
    public SceneWaveObjectSelection,
    public SceneTranslationSelection,
    public ScenePaletteSelection,
    public SceneImageSelection {
    const char* nameValue;

public:
    explicit FakeSceneSelection(const char* name_)
        : nameValue(name_) { }

    virtual const char* catalogName() const { return nameValue; }
    virtual const char* currentName() const { return nameValue; }
    virtual int currentValue() const { return 0; }
    virtual int entryCount() const { return 0; }
    virtual void change(int) { }
    virtual void change(const char*, RandomSource&) { }
    virtual int changeRandom(RandomSource&) { return 0; }
    virtual void setValue(int) { }
    virtual const Flame* currentFlame() { return NULL; }
    virtual int encodedValue() const { return 0; }
    virtual const char* selectionText() const { return nameValue; }
    virtual Wave* currentWave() { return NULL; }
    virtual WObject* currentObject() { return NULL; }
    virtual TranslationTable currentTranslationTable() {
        return TranslationTable();
    }
    virtual PaletteEntry* currentPaletteEntry() { return NULL; }
    virtual const IndexedImage* currentImage() { return NULL; }
};

class FakeSceneVisualSelections : public SceneVisualSelections {
public:
    FakeSceneSelection flameValue;
    FakeSceneSelection generalFlameValue;
    FakeSceneSelection waveValue;
    FakeSceneSelection waveScaleValue;
    FakeSceneSelection tableValue;
    FakeSceneSelection objectValue;
    FakeSceneSelection translationValue;
    FakeSceneSelection paletteValue;
    FakeSceneSelection borderValue;
    FakeSceneSelection flashlightValue;
    FakeSceneSelection imageValue;

    FakeSceneVisualSelections()
        : flameValue("flame")
        , generalFlameValue("general-flame")
        , waveValue("wave")
        , waveScaleValue("wave-scale")
        , tableValue("table")
        , objectValue("object")
        , translationValue("translation")
        , paletteValue("palette")
        , borderValue("border")
        , flashlightValue("flashlight")
        , imageValue("image") { }

    virtual SceneFlameSelection& flame() { return flameValue; }
    virtual SceneGeneralFlameSelection& generalFlame() {
        return generalFlameValue;
    }
    virtual SceneWaveSelection& wave() { return waveValue; }
    virtual SceneOptionSelection& waveScale() { return waveScaleValue; }
    virtual SceneOptionSelection& table() { return tableValue; }
    virtual SceneOptionSelection& object() { return objectValue; }
    virtual SceneTranslationSelection& translation() {
        return translationValue;
    }
    virtual ScenePaletteSelection& palette() { return paletteValue; }
    virtual SceneOptionSelection& border() { return borderValue; }
    virtual SceneOptionSelection& flashlight() { return flashlightValue; }
    virtual SceneImageSelection& images() { return imageValue; }
};

class FakeMillisecondClock : public MillisecondClock {
public:
    int value;

    explicit FakeMillisecondClock(int value_)
        : value(value_) { }

    virtual int milliseconds() const { return value; }
};

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
void Interface::run(InterfaceRuntime& runtime, InputQueue&,
    KeymapRegistry&, CommandRegistry&, CommandDispatcher&, CommandContext&) {
    (void)runtime;
    preRun();
}

InterfaceElementOption::InterfaceElementOption(const char* t, Option* o,
    int i1, int i2, int i3)
    : InterfaceElement(t)
    , opt(o)
    , inc1(i1)
    , inc2(i2)
    , inc3(i3) { }
const char* InterfaceElementOption::text(InterfaceRuntime&,
    const OverlayRenderContext&, int) { return str; }
int InterfaceElementOption::doKey(InterfaceRuntime&, KeymapRegistry&,
    CommandRegistry&, CommandDispatcher&, CommandContext&, int) {
    return 1;
}

CommandRegistry::CommandRegistry()
    : firstValue(NULL) { }
CommandRegistry::~CommandRegistry() { }
void CommandRegistry::registerAction(Action*) { }
Action* CommandRegistry::find(const char*) const { return NULL; }
Action* CommandRegistry::findLongestPrefix(const char*) const { return NULL; }

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
void CommandContext::targetOption(Option& option,
    InterfaceElementOption& element) {
    optionValue = &option;
    effectControlValue = NULL;
    hasSceneTargetValue = 0;
    optionElementValue = &element;
    effectChoiceIndexValue = -1;
}
void CommandContext::targetEffectControl(EffectControl& effectControl,
    InterfaceElementOption& element) {
    optionValue = &effectControl;
    effectControlValue = &effectControl;
    hasSceneTargetValue = 0;
    optionElementValue = &element;
    effectChoiceIndexValue = -1;
}
void CommandContext::targetEffectChoice(EffectControl& effectControl,
    Option& option, int selectedIndex) {
    optionValue = &option;
    effectControlValue = &effectControl;
    hasSceneTargetValue = 0;
    optionElementValue = NULL;
    effectChoiceIndexValue = selectedIndex;
}
void CommandContext::targetSceneSelection(RuntimeSceneTarget sceneTarget,
    InterfaceElementOption& element) {
    optionValue = NULL;
    effectControlValue = NULL;
    sceneTargetValue = sceneTarget;
    hasSceneTargetValue = 1;
    optionElementValue = &element;
    effectChoiceIndexValue = -1;
}
void CommandContext::targetSceneChoice(
    RuntimeSceneTarget sceneTarget, int selectedIndex) {
    optionValue = NULL;
    effectControlValue = NULL;
    sceneTargetValue = sceneTarget;
    hasSceneTargetValue = 1;
    optionElementValue = NULL;
    effectChoiceIndexValue = selectedIndex;
}
void CommandContext::changeValueByElementIncrement(int incrementIndex,
    double value) {
    if (optionElementValue == NULL)
        return;

    int increment = 0;
    if (incrementIndex == 1)
        increment = optionElementValue->inc1;
    else if (incrementIndex == 2)
        increment = optionElementValue->inc2;
    else if (incrementIndex == 3)
        increment = optionElementValue->inc3;

    if (increment == 0)
        return;

    int step = int(value * increment);
    if ((hasSceneTargetValue != 0) && (runtimeCommandSinkValue != NULL)) {
        runtimeCommandSinkValue->apply(
            RuntimeCommand::changeSceneBy(sceneTargetValue, step));
    } else if ((effectControlValue != NULL) && (commandRouterValue != NULL)) {
        commandRouterValue->changeEffectControlBy(*effectControlValue, step);
    } else if ((optionValue != NULL) && (commandRouterValue != NULL)) {
        commandRouterValue->changeOptionBy(*optionValue, step);
    }
}
void CommandContext::setValueFromElement(double) { }
void CommandContext::toggleEffectControlLock() {
    if ((hasSceneTargetValue != 0) && (runtimeCommandSinkValue != NULL)) {
        runtimeCommandSinkValue->apply(
            RuntimeCommand::toggleSceneLock(sceneTargetValue));
    }
}
void CommandContext::toggleEffectChoiceUse() {
    if ((hasSceneTargetValue != 0) && (effectChoiceIndexValue >= 0)
        && (runtimeCommandSinkValue != NULL)) {
        runtimeCommandSinkValue->apply(RuntimeCommand::toggleSceneChoiceUse(
            sceneTargetValue, effectChoiceIndexValue));
    }
}
void CommandContext::activateEffectChoice() {
    if ((hasSceneTargetValue != 0) && (effectChoiceIndexValue >= 0)
        && (runtimeCommandSinkValue != NULL)) {
        runtimeCommandSinkValue->apply(RuntimeCommand::activateScene(
            sceneTargetValue, effectChoiceIndexValue));
    }
}

int CommandDispatcher::dispatch(
    const std::vector<ActionInvocation>&, CommandContext&) const {
    return 1;
}
int CommandDispatcher::dispatchKeymap(KeymapRegistry&, CommandRegistry&,
    const char*, int, CommandContext& context) const {
    context.changeValueByElementIncrement(1, 1.0);
    return 0;
}

KeymapRegistry::KeymapRegistry()
    : firstValue(NULL) { }
KeymapRegistry::~KeymapRegistry() { }

class CapturingTargetRouter : public RuntimeCommandTargetRouter {
public:
    int optionByCalls;
    Option* lastOption;
    int lastValue;

    CapturingTargetRouter()
        : optionByCalls(0)
        , lastOption(NULL)
        , lastValue(0) { }

    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl&, int) {
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl&, const char*) {
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet activateEffectControl(
        EffectControl&, int) {
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet toggleEffectChoiceUse(
        EffectControl&, int) {
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet changeOptionBy(Option& option, int by) {
        optionByCalls++;
        lastOption = &option;
        lastValue = by;
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet changeOptionTo(Option&, const char*) {
        return RuntimeChangeSet();
    }

    virtual RuntimeChangeSet toggleEffectControlLock(EffectControl&) {
        return RuntimeChangeSet();
    }
};

class CapturingCommandSink : public RuntimeCommandSink {
public:
    int calls;
    RuntimeCommand lastCommand;

    CapturingCommandSink()
        : calls(0)
        , lastCommand(RuntimeCommandRequestClose) { }

    virtual RuntimeChangeSet apply(const RuntimeCommand& command) {
        calls++;
        lastCommand = command;
        return RuntimeChangeSet();
    }
};

static void testSelectionIsOwnedByRuntime() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);
    Interface main("main", "Main", NULL);
    Interface other("other", "Other", NULL);

    main.nElements = 3;
    runtime.registerInterface(main);
    runtime.registerInterface(other);

    assert(runtime.find("main") == &main);
    assert(runtime.find("other") == &other);
    assert(runtime.find("missing") == NULL);

    runtime.registerOwnedInterface(new Interface("owned", "Owned", NULL));
    assert(runtime.find("owned") != NULL);

    runtime.set("main");
    assert(runtime.current() == &main);
    runtime.moveSelectionBy(10);
    assert(main.sel == 2);
    assert(other.sel == -1);

    runtime.set("other");
    assert(runtime.current() == &other);
    assert(other.sel == -1);
}

static void testStatusAndPresetFlagsAreOwnedByRuntime() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);

    assert(runtime.showStatus() == 0);
    runtime.toggleStatus();
    assert(runtime.showStatus() == 1);

    assert(runtime.saveToPreset() == 0);
    runtime.toggleSaveToPreset();
    assert(runtime.saveToPreset() == 1);
    runtime.clearSaveToPreset();
    assert(runtime.saveToPreset() == 0);

    assert(strcmp(runtime.extraKeymap(), "") == 0);
    runtime.setExtraKeymap("temporary");
    assert(strcmp(runtime.extraKeymap(), "temporary") == 0);
    runtime.setExtraKeymap(NULL);
    assert(strcmp(runtime.extraKeymap(), "") == 0);
}

static void testHelpScrollStateIsOwnedByRuntime() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);

    assert(runtime.helpScrolling() == 0);
    assert(runtime.helpScrollPosition() == 0.0);

    runtime.toggleHelpScrolling();
    assert(runtime.helpScrolling() == 1);
    runtime.advanceHelpScroll(2.5);
    assert(runtime.helpScrollPosition() == 2.5);
    assert(runtime.helpScrolling() == 1);

    runtime.scrollHelpBy(-3.0, 10);
    assert(runtime.helpScrollPosition() == 9.5);
    assert(runtime.helpScrolling() == 0);
}

static void testCreditsAnimationStateIsOwnedByRuntime() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);

    assert(runtime.creditsPosition() == 0.0);
    assert(runtime.updateCreditsPosition(1000, 20) == -16.0);
    assert(runtime.creditsPosition() == -16.0);
    assert(runtime.updateCreditsPosition(1500, 20) == -14.0);
}

static void testOptionCommandContextIsScoped() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);
    CapturingTargetRouter router;
    SimpleOption option;
    InterfaceElementOption element("value: %s", &option, 5, 10, 20);
    KeymapRegistry keymaps;
    CommandRegistry commands;
    CommandDispatcher dispatcher;
    CommandContext context(runtime, NULL, &router);

    assert(runtime.runOptionKey(option, element, keymaps, commands,
        dispatcher, context, "test", 'x') == 0);
    assert(router.optionByCalls == 1);
    assert(router.lastOption == &option);
    assert(router.lastValue == 5);

    context.changeValueByElementIncrement(1, 1.0);
    assert(router.optionByCalls == 1);
}

static void testSceneSelectionCommandContextUsesTypedRuntimeCommand() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);
    CapturingCommandSink sink;
    CapturingTargetRouter router;
    SimpleOption fallback;
    InterfaceElementOption element("scene: %s", &fallback, 5, 10, 20);
    KeymapRegistry keymaps;
    CommandRegistry commands;
    CommandDispatcher dispatcher;
    CommandContext context(runtime, &sink, &router);

    assert(runtime.runSceneSelectionKey(RuntimeScenePalette, element,
        keymaps, commands, dispatcher, context, "test", "option", 'p') == 0);
    assert(sink.calls == 1);
    assert(sink.lastCommand.type == RuntimeCommandChangeSceneBy);
    assert(sink.lastCommand.sceneTarget == RuntimeScenePalette);
    assert(sink.lastCommand.value == 5);
    assert(router.optionByCalls == 0);

    context.changeValueByElementIncrement(1, 1.0);
    assert(sink.calls == 1);
}

static void testSceneVisualSelectionsAreLateBoundToRuntime() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);
    FakeSceneVisualSelections selections;

    assert(runtime.sceneSelection(RuntimeScenePalette) == NULL);

    runtime.setSceneVisualSelections(&selections);
    assert(runtime.sceneSelection(RuntimeSceneFlame)
        == &selections.flameValue);
    assert(runtime.sceneSelection(RuntimeSceneWave)
        == &selections.waveValue);
    assert(runtime.sceneSelection(RuntimeSceneWaveScale)
        == &selections.waveScaleValue);
    assert(runtime.sceneSelection(RuntimeSceneObject)
        == &selections.objectValue);
    assert(runtime.sceneSelection(RuntimeSceneTranslation)
        == &selections.translationValue);
    assert(runtime.sceneSelection(RuntimeScenePalette)
        == &selections.paletteValue);
    assert(runtime.sceneSelection(RuntimeSceneTable)
        == &selections.tableValue);
    assert(runtime.sceneSelection(RuntimeSceneImage)
        == &selections.imageValue);

    runtime.setSceneVisualSelections(NULL);
    assert(runtime.sceneSelection(RuntimeScenePalette) == NULL);
}

static void testSceneChoiceCommandContextUsesTypedRuntimeCommand() {
    FakeMillisecondClock clock(1000);
    InterfaceRuntime runtime(clock);
    CapturingCommandSink sink;
    CapturingTargetRouter router;
    CommandContext context(runtime, &sink, &router);

    context.targetSceneChoice(RuntimeSceneImage, 4);
    context.activateEffectChoice();
    assert(sink.calls == 1);
    assert(sink.lastCommand.type == RuntimeCommandActivateScene);
    assert(sink.lastCommand.sceneTarget == RuntimeSceneImage);
    assert(sink.lastCommand.value == 4);

    context.toggleEffectChoiceUse();
    assert(sink.calls == 2);
    assert(sink.lastCommand.type == RuntimeCommandToggleSceneChoiceUse);
    assert(sink.lastCommand.sceneTarget == RuntimeSceneImage);
    assert(sink.lastCommand.value == 4);
}

static void testMillisecondsComeFromInjectedClock() {
    FakeMillisecondClock clock(1234);
    InterfaceRuntime runtime(clock);

    assert(runtime.milliseconds() == 1234);
    clock.value = 5678;
    assert(runtime.milliseconds() == 5678);
}

int main() {
    testSelectionIsOwnedByRuntime();
    testStatusAndPresetFlagsAreOwnedByRuntime();
    testHelpScrollStateIsOwnedByRuntime();
    testCreditsAnimationStateIsOwnedByRuntime();
    testOptionCommandContextIsScoped();
    testSceneSelectionCommandContextUsesTypedRuntimeCommand();
    testSceneVisualSelectionsAreLateBoundToRuntime();
    testSceneChoiceCommandContextUsesTypedRuntimeCommand();
    testMillisecondsComeFromInjectedClock();
    return 0;
}
