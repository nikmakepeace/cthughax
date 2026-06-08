/** @file
 * Application-owned interface runtime state and command context.
 */

#include "cthugha.h"
#include "InterfaceRuntime.h"

#include "EffectControl.h"
#include "InputQueue.h"
#include "Interface.h"
#include "ProcessServices.h"
#include "RuntimeCommandTargets.h"
#include "SceneVisualSelections.h"
#include "keymap.h"

#include <string.h>
#include <strings.h>

InterfaceRuntime::InterfaceRuntime(MillisecondClock& clock_)
    : currentInterfaceValue(NULL)
    , runtimeConfigRegistryValue(NULL)
    , sceneVisualSelectionsValue(NULL)
    , sceneChangeStatusProviderValue(NULL)
    , autoChangeControlsValue(NULL)
    , audioProcessingSelectorValue(NULL)
    , clock(clock_)
    , saveToPresetValue(0)
    , showStatusValue(0)
    , helpScrollPositionValue(0.0)
    , helpScrollingValue(0)
    , creditsPositionValue(0.0)
    , creditsFirstTimeValue(-1) {
    extraKeymapValue[0] = '\0';
}

InterfaceRuntime::~InterfaceRuntime() {
    for (std::vector<Interface*>::iterator it = ownedInterfacesValue.begin();
         it != ownedInterfacesValue.end(); ++it)
        delete *it;
}

void InterfaceRuntime::registerInterface(Interface& interface_) {
    for (std::vector<Interface*>::iterator it = interfacesValue.begin();
         it != interfacesValue.end(); ++it) {
        if (*it == &interface_)
            return;
    }

    interfacesValue.push_back(&interface_);
}

void InterfaceRuntime::registerOwnedInterface(Interface* interface_) {
    if (interface_ == NULL)
        return;

    ownedInterfacesValue.push_back(interface_);
    registerInterface(*interface_);
}

Interface* InterfaceRuntime::find(const char* name) {
    if (name == NULL)
        return NULL;

    for (std::vector<Interface*>::iterator it = interfacesValue.begin();
         it != interfacesValue.end(); ++it) {
        Interface* candidate = *it;
        if (strcasecmp(name, candidate->name) == 0)
            return candidate;
    }

    return NULL;
}

const Interface* InterfaceRuntime::find(const char* name) const {
    if (name == NULL)
        return NULL;

    for (std::vector<Interface*>::const_iterator it = interfacesValue.begin();
         it != interfacesValue.end(); ++it) {
        const Interface* candidate = *it;
        if (strcasecmp(name, candidate->name) == 0)
            return candidate;
    }

    return NULL;
}

void InterfaceRuntime::set(const char* name) {
    if (name == NULL)
        return;

    Interface* candidate = find(name);
    if (candidate != NULL) {
        currentInterfaceValue = candidate;
        clampCurrentSelection();
        return;
    }

    CTH_ERROR("Unknown interface '%s'\n", name);
}

void InterfaceRuntime::runCurrent(InputQueue& inputQueue,
    KeymapRegistry& keymaps, CommandRegistry& commands,
    CommandDispatcher& dispatcher, CommandContext& context) {
    if (currentInterfaceValue != NULL)
        currentInterfaceValue->run(*this, inputQueue, keymaps, commands,
            dispatcher, context);
}

void InterfaceRuntime::clampCurrentSelection() {
    if (currentInterfaceValue == NULL)
        return;

    if (currentInterfaceValue->sel < -1)
        currentInterfaceValue->sel = -1;
    if (currentInterfaceValue->sel >= currentInterfaceValue->nElements)
        currentInterfaceValue->sel = currentInterfaceValue->nElements - 1;
}

void InterfaceRuntime::moveSelectionBy(int by) {
    if (currentInterfaceValue == NULL)
        return;

    currentInterfaceValue->sel += by;
    clampCurrentSelection();
}

void InterfaceRuntime::setSelection(int selection) {
    if (currentInterfaceValue == NULL)
        return;

    currentInterfaceValue->sel = selection;
    clampCurrentSelection();
}

void InterfaceRuntime::selectHome() {
    setSelection(-1);
}

void InterfaceRuntime::selectEnd() {
    if (currentInterfaceValue != NULL)
        setSelection(currentInterfaceValue->nElements - 1);
}

void InterfaceRuntime::selectNextInList(const char* const* names) {
    if ((currentInterfaceValue == NULL) || (names == NULL))
        return;

    for (const char* const* name = names; *name != NULL; name++) {
        if (strcasecmp(*name, currentInterfaceValue->name) == 0) {
            if (name[1] != NULL)
                set(name[1]);
            return;
        }
    }
}

void InterfaceRuntime::selectPreviousInList(const char* const* names) {
    if ((currentInterfaceValue == NULL) || (names == NULL))
        return;

    for (const char* const* name = names + 1; *name != NULL; name++) {
        if (strcasecmp(*name, currentInterfaceValue->name) == 0) {
            set(name[-1]);
            return;
        }
    }
}

void InterfaceRuntime::setRuntimeConfigRegistry(RuntimeConfigRegistry* registry) {
    runtimeConfigRegistryValue = registry;
}

const RuntimeConfigRegistry* InterfaceRuntime::runtimeConfigRegistry() const {
    return runtimeConfigRegistryValue;
}

void InterfaceRuntime::setSceneVisualSelections(
    SceneVisualSelections* selections) {
    sceneVisualSelectionsValue = selections;
}

SceneOptionSelection* InterfaceRuntime::sceneSelection(
    RuntimeSceneTarget target) const {
    if (sceneVisualSelectionsValue == NULL)
        return NULL;

    switch (target) {
    case RuntimeSceneFlame:
        return &sceneVisualSelectionsValue->flame();
    case RuntimeSceneGeneralFlame:
        return &sceneVisualSelectionsValue->generalFlame();
    case RuntimeSceneWave:
        return &sceneVisualSelectionsValue->wave();
    case RuntimeSceneWaveScale:
        return &sceneVisualSelectionsValue->waveScale();
    case RuntimeSceneObject:
        return &sceneVisualSelectionsValue->object();
    case RuntimeSceneTranslation:
        return &sceneVisualSelectionsValue->translation();
    case RuntimeSceneBorder:
        return &sceneVisualSelectionsValue->border();
    case RuntimeSceneFlashlight:
        return &sceneVisualSelectionsValue->flashlight();
    case RuntimeScenePalette:
        return &sceneVisualSelectionsValue->palette();
    case RuntimeSceneTable:
        return &sceneVisualSelectionsValue->table();
    case RuntimeSceneImage:
        return &sceneVisualSelectionsValue->images();
    }

    return NULL;
}

void InterfaceRuntime::setSceneChangeStatusProvider(
    const SceneChangeStatusProvider* provider) {
    sceneChangeStatusProviderValue = provider;
}

const SceneChangeStatusProvider* InterfaceRuntime::sceneChangeStatusProvider() const {
    return sceneChangeStatusProviderValue;
}

void InterfaceRuntime::setAutoChangeControls(AutoChangeControls* controls) {
    autoChangeControlsValue = controls;
}

AutoChangeControls* InterfaceRuntime::autoChangeControls() const {
    return autoChangeControlsValue;
}

void InterfaceRuntime::setAudioProcessingSelector(AudioProcessingSelector* selector) {
    audioProcessingSelectorValue = selector;
}

AudioProcessingSelector* InterfaceRuntime::audioProcessingSelector() const {
    return audioProcessingSelectorValue;
}

void InterfaceRuntime::toggleSaveToPreset() {
    saveToPresetValue = 1 - saveToPresetValue;
}

void InterfaceRuntime::clearSaveToPreset() {
    saveToPresetValue = 0;
}

int InterfaceRuntime::saveToPreset() const {
    return saveToPresetValue;
}

void InterfaceRuntime::toggleStatus() {
    showStatusValue = 1 - showStatusValue;
}

int InterfaceRuntime::showStatus() const {
    return showStatusValue;
}

void InterfaceRuntime::setExtraKeymap(const char* name) {
    if (name == NULL) {
        extraKeymapValue[0] = '\0';
        return;
    }

    strncpy(extraKeymapValue, name, sizeof(extraKeymapValue) - 1);
    extraKeymapValue[sizeof(extraKeymapValue) - 1] = '\0';
}

const char* InterfaceRuntime::extraKeymap() const {
    return extraKeymapValue;
}

void InterfaceRuntime::toggleHelpScrolling() {
    helpScrollingValue = 1 - helpScrollingValue;
}

void InterfaceRuntime::scrollHelpBy(double by, int lineCount) {
    helpScrollPositionValue += by;
    if ((helpScrollPositionValue < 0.0) && (lineCount > 0))
        helpScrollPositionValue += lineCount;
    helpScrollingValue = 0;
}

void InterfaceRuntime::advanceHelpScroll(double by) {
    helpScrollPositionValue += by;
}

double InterfaceRuntime::helpScrollPosition() const {
    return helpScrollPositionValue;
}

int InterfaceRuntime::helpScrolling() const {
    return helpScrollingValue;
}

double InterfaceRuntime::updateCreditsPosition(int currentTime, int textHeight) {
    if (creditsFirstTimeValue == -1)
        creditsFirstTimeValue = currentTime;

    int timeDiff = currentTime - creditsFirstTimeValue;
    creditsPositionValue = -(double(textHeight) * 0.8) + double(timeDiff) / 250.0;
    return creditsPositionValue;
}

double InterfaceRuntime::creditsPosition() const {
    return creditsPositionValue;
}

int InterfaceRuntime::milliseconds() const {
    return clock.milliseconds();
}

int InterfaceRuntime::runOptionKey(Option& option, InterfaceElementOption& element,
    KeymapRegistry& keymaps, CommandRegistry& commands,
    CommandDispatcher& dispatcher, CommandContext& baseContext,
    const char* keymapName, int key) {
    CommandContext context = baseContext;
    context.targetOption(option, element);
    int result = dispatcher.dispatchKeymap(keymaps, commands, keymapName, key,
        context);
    return result;
}

int InterfaceRuntime::runEffectControlKey(EffectControl& effectControl,
    InterfaceElementOption& element, KeymapRegistry& keymaps,
    CommandRegistry& commands, CommandDispatcher& dispatcher,
    CommandContext& baseContext, const char* effectControlKeymapName,
    const char* optionKeymapName, int key) {
    CommandContext context = baseContext;
    context.targetEffectControl(effectControl, element);
    int result = dispatcher.dispatchKeymap(keymaps, commands,
        effectControlKeymapName, key, context);
    if (result == 1)
        result = dispatcher.dispatchKeymap(keymaps, commands,
            optionKeymapName, key, context);

    return result;
}

int InterfaceRuntime::runSceneSelectionKey(RuntimeSceneTarget sceneTarget,
    InterfaceElementOption& element, KeymapRegistry& keymaps,
    CommandRegistry& commands, CommandDispatcher& dispatcher,
    CommandContext& baseContext, const char* sceneSelectionKeymapName,
    const char* optionKeymapName, int key) {
    CommandContext context = baseContext;
    context.targetSceneSelection(sceneTarget, element);
    int result = dispatcher.dispatchKeymap(keymaps, commands,
        sceneSelectionKeymapName, key, context);
    if (result == 1)
        result = dispatcher.dispatchKeymap(keymaps, commands,
            optionKeymapName, key, context);

    return result;
}

int InterfaceRuntime::runEffectChoiceKey(EffectControl& effectControl,
    Option& option, KeymapRegistry& keymaps, CommandRegistry& commands,
    CommandDispatcher& dispatcher, CommandContext& baseContext,
    int selectedIndex, int key) {
    CommandContext context = baseContext;
    context.targetEffectChoice(effectControl, option, selectedIndex);
    int result = dispatcher.dispatchKeymap(keymaps, commands, "ListOption",
        key, context);
    if (result)
        result = dispatcher.dispatchKeymap(keymaps, commands, "Option", key,
            context);

    return result;
}

int InterfaceRuntime::runSceneChoiceKey(RuntimeSceneTarget sceneTarget,
    KeymapRegistry& keymaps, CommandRegistry& commands,
    CommandDispatcher& dispatcher, CommandContext& baseContext,
    int selectedIndex, int key) {
    CommandContext context = baseContext;
    context.targetSceneChoice(sceneTarget, selectedIndex);
    int result = dispatcher.dispatchKeymap(keymaps, commands, "ListOption",
        key, context);
    if (result)
        result = dispatcher.dispatchKeymap(keymaps, commands, "Option", key,
            context);

    return result;
}
