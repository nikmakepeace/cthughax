/** @file
 * Owned runtime state for interface selection, adapters, and key context.
 */

#ifndef __INTERFACE_RUNTIME_H
#define __INTERFACE_RUNTIME_H

#include "RuntimeCommand.h"

#include <vector>

class SceneChangeStatusProvider;
class AutoChangeControls;
class AudioProcessingSelector;
class CommandContext;
class CommandDispatcher;
class CommandRegistry;
class EffectControl;
class Interface;
class InterfaceElementOption;
class InputQueue;
class KeymapRegistry;
class MillisecondClock;
class Option;
class RuntimeConfigRegistry;
class SceneOptionSelection;
class SceneVisualSelections;
/**
 * Application-owned runtime state for the keyboard/interface subsystem.
 *
 * Concrete Interface objects are still static panel definitions in this pass,
 * but the live registry, selected interface, runtime adapters, status flags,
 * and option-command target are owned by this object.
 */
class InterfaceRuntime {
    std::vector<Interface*> interfacesValue;
    std::vector<Interface*> ownedInterfacesValue;
    Interface* currentInterfaceValue;
    RuntimeConfigRegistry* runtimeConfigRegistryValue;
    SceneVisualSelections* sceneVisualSelectionsValue;
    const SceneChangeStatusProvider* sceneChangeStatusProviderValue;
    AutoChangeControls* autoChangeControlsValue;
    AudioProcessingSelector* audioProcessingSelectorValue;
    MillisecondClock& clock;
    int saveToPresetValue;
    int showStatusValue;
    char extraKeymapValue[512];
    double helpScrollPositionValue;
    int helpScrollingValue;
    double creditsPositionValue;
    int creditsFirstTimeValue;

    void clampCurrentSelection();

public:
    /**
     * Creates an empty interface runtime with no selected panel.
     *
     * @param clock_ Application-owned millisecond clock for UI timers.
     */
    explicit InterfaceRuntime(MillisecondClock& clock_);

    /** Destroys interface panels owned through registerOwnedInterface(). */
    ~InterfaceRuntime();

    /**
     * Registers a concrete interface panel with this runtime.
     *
     * @param interface_ Panel definition to register; it must outlive the
     *        runtime.
     */
    void registerInterface(Interface& interface_);

    /**
     * Takes ownership of a concrete interface panel and registers it.
     *
     * @param interface_ Newly allocated panel; NULL is ignored.
     */
    void registerOwnedInterface(Interface* interface_);

    /**
     * Finds a registered interface by name without selecting it.
     *
     * @param name Case-insensitive interface name.
     * @return Matching interface, or NULL when no panel has that name.
     */
    Interface* find(const char* name);

    /**
     * Finds a registered interface by name without selecting it.
     *
     * @param name Case-insensitive interface name.
     * @return Matching interface, or NULL when no panel has that name.
     */
    const Interface* find(const char* name) const;

    /**
     * Selects a registered interface by name.
     *
     * @param name Case-insensitive interface name.
     */
    void set(const char* name);

    /** @return Currently selected interface, or NULL before selection. */
    Interface* current() { return currentInterfaceValue; }

    /** @return Currently selected interface, or NULL before selection. */
    const Interface* current() const { return currentInterfaceValue; }

    /** Services the selected interface once when one is selected. */
    void runCurrent(InputQueue& inputQueue, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context);

    /**
     * Moves the selected row in the active interface.
     *
     * @param by Signed row offset.
     */
    void moveSelectionBy(int by);

    /**
     * Sets the selected row in the active interface.
     *
     * @param selection Row index, or -1 for the title/header row.
     */
    void setSelection(int selection);

    /** Selects the title/header row in the active interface. */
    void selectHome();

    /** Selects the last element row in the active interface. */
    void selectEnd();

    /**
     * Selects the next named interface in the supplied navigation list.
     *
     * @param names NULL-terminated ordered list of interface names.
     */
    void selectNextInList(const char* const* names);

    /**
     * Selects the previous named interface in the supplied navigation list.
     *
     * @param names NULL-terminated ordered list of interface names.
     */
    void selectPreviousInList(const char* const* names);

    /**
     * Installs the registry used for displaying current runtime selections.
     *
     * @param registry Registry to read; NULL restores direct option fallbacks.
     */
    void setRuntimeConfigRegistry(RuntimeConfigRegistry* registry);

    /**
     * Returns the registry used for displaying current runtime selections.
     *
     * @return Installed registry, or NULL when not yet available.
     */
    const RuntimeConfigRegistry* runtimeConfigRegistry() const;

    /**
     * Installs the read-only source used by Scene-targeted visual list panels.
     *
     * @param selections Native Scene visual selections, or NULL to fall back to
     *        legacy list display sources.
     */
    void setSceneVisualSelections(SceneVisualSelections* selections);

    /**
     * Returns the native Scene selection for a runtime scene target.
     *
     * @param target Runtime scene target to display.
     * @return Selection owned by SceneRuntime, or NULL before Scene startup.
     */
    SceneOptionSelection* sceneSelection(RuntimeSceneTarget target) const;

    /**
     * Installs the provider used for automatic scene-change status text.
     *
     * @param provider Provider to read; NULL hides auto-change status.
     */
    void setSceneChangeStatusProvider(
        const SceneChangeStatusProvider* provider);

    /**
     * Returns the provider used for automatic scene-change status text.
     *
     * @return Installed provider, or NULL when automatic scene changes are unavailable.
     */
    const SceneChangeStatusProvider* sceneChangeStatusProvider() const;

    /**
     * Installs Option adapters for automatic scene-change panel controls.
     *
     * @param controls Controls to read; NULL disables those panel rows.
     */
    void setAutoChangeControls(AutoChangeControls* controls);

    /**
     * Returns Option adapters for automatic scene-change panel controls.
     *
     * @return Installed controls, or NULL before runtime setup.
     */
    AutoChangeControls* autoChangeControls() const;

    /**
     * Installs the selector used for the audio-processing option panel row.
     *
     * @param selector Selector to read and mutate; NULL disables the row.
     */
    void setAudioProcessingSelector(AudioProcessingSelector* selector);

    /**
     * Returns the selector used for the audio-processing option panel row.
     *
     * @return Installed selector, or NULL before runtime setup.
     */
    AudioProcessingSelector* audioProcessingSelector() const;

    /** Toggles whether number keys save or restore presets. */
    void toggleSaveToPreset();

    /** Clears save-to-preset mode after a preset save has been requested. */
    void clearSaveToPreset();

    /** @return Nonzero when number keys should save presets. */
    int saveToPreset() const;

    /** Toggles runtime status overlay visibility. */
    void toggleStatus();

    /** @return Nonzero when status text should be displayed. */
    int showStatus() const;

    /**
     * Sets the extra keymap that the main interface should try before its
     * normal keymaps.
     *
     * @param name Keymap name, or NULL to clear.
     */
    void setExtraKeymap(const char* name);

    /** @return Extra keymap name for the main interface, or an empty string. */
    const char* extraKeymap() const;

    /** Toggles automatic help text scrolling. */
    void toggleHelpScrolling();

    /**
     * Moves the help scroll position manually and disables auto scrolling.
     *
     * @param by Signed line offset.
     * @param lineCount Number of help lines, used to preserve wrap behavior.
     */
    void scrollHelpBy(double by, int lineCount);

    /**
     * Moves the help scroll position while preserving current scrolling mode.
     *
     * @param by Signed line offset.
     */
    void advanceHelpScroll(double by);

    /** @return Current help scroll position in lines. */
    double helpScrollPosition() const;

    /** @return Nonzero when help text should auto-scroll. */
    int helpScrolling() const;

    /**
     * Updates and returns credits animation position for the current frame.
     *
     * @param currentTime Current UI clock time in milliseconds.
     * @param textHeight Text viewport height in rows.
     * @return Current credits scroll position in lines.
     */
    double updateCreditsPosition(int currentTime, int textHeight);

    /** @return Current credits scroll position in lines. */
    double creditsPosition() const;

    /** @return Current Application-owned UI time in milliseconds. */
    int milliseconds() const;

    /**
     * Runs a key through an option element with a scoped command context.
     *
     * @param option Option targeted by option-element actions.
     * @param element Element supplying increment sizes for value actions.
     * @param keymapName Keymap that should handle the key.
     * @param key Key code to route.
     * @return Keymap handling result.
     */
    int runOptionKey(Option& option, InterfaceElementOption& element,
        KeymapRegistry& keymaps, CommandRegistry& commands,
        CommandDispatcher& dispatcher, CommandContext& context,
        const char* keymapName, int key);

    /**
     * Runs a key through an effect-control element with scoped command context.
     *
     * @param effectControl Effect control targeted by effect actions.
     * @param element Element supplying increment sizes for value actions.
     * @param effectControlKeymapName First keymap to try.
     * @param optionKeymapName Fallback option keymap.
     * @param key Key code to route.
     * @return Keymap handling result.
     */
    int runEffectControlKey(EffectControl& effectControl,
        InterfaceElementOption& element, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context, const char* effectControlKeymapName,
        const char* optionKeymapName, int key);

    /**
     * Runs a key through a typed Scene selection row.
     *
     * @param sceneTarget Scene selection targeted by visual-row actions.
     * @param element Element supplying increment sizes for value actions.
     * @param sceneSelectionKeymapName First keymap to try.
     * @param optionKeymapName Fallback option keymap.
     * @param key Key code to route.
     * @return Keymap handling result.
     */
    int runSceneSelectionKey(RuntimeSceneTarget sceneTarget,
        InterfaceElementOption& element, KeymapRegistry& keymaps,
        CommandRegistry& commands, CommandDispatcher& dispatcher,
        CommandContext& context, const char* sceneSelectionKeymapName,
        const char* optionKeymapName, int key);

    /**
     * Runs a key through an effect-choice list row with scoped command context.
     *
     * @param effectControl Effect control owning the selected row.
     * @param option Use-option for the selected row.
     * @param key Key code to route.
     * @return Keymap handling result.
     */
    int runEffectChoiceKey(EffectControl& effectControl, Option& option,
        KeymapRegistry& keymaps, CommandRegistry& commands,
        CommandDispatcher& dispatcher, CommandContext& context,
        int selectedIndex, int key);

    /**
     * Runs a key through a typed Scene choice-list row.
     *
     * @param sceneTarget Scene selection targeted by list actions.
     * @param key Key code to route.
     * @return Keymap handling result.
     */
    int runSceneChoiceKey(RuntimeSceneTarget sceneTarget,
        KeymapRegistry& keymaps, CommandRegistry& commands,
        CommandDispatcher& dispatcher, CommandContext& context,
        int selectedIndex, int key);
};

#endif
