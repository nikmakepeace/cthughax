#ifndef CTHUGHA_COMMANDS_INPUT_RUNTIME_H
#define CTHUGHA_COMMANDS_INPUT_RUNTIME_H

#include "InputQueue.h"
#include "Interface.h"
#include "keymap.h"

#include <memory>

class DisplayPresentationSettings;
class MillisecondClock;
class Option;
struct InputConfig;

/**
 * Commands/Input module root owned by Application.
 *
 * This groups input translation, keymaps, command registration/dispatch, and
 * interface runtime state behind one explicit dependency boundary.
 */
class CommandsInputRuntime {
    InputQueue inputQueueValue;
    CommandRegistry commandsValue;
    CommandDispatcher dispatcherValue;
    KeymapRegistry keymapsValue;
    std::unique_ptr<InterfaceRuntime> interfaceRuntimeValue;
    std::unique_ptr<ErrorMessages> errorMessagesValue;

public:
    /**
     * Creates input, command, keymap, and interface runtime state.
     *
     * @param clock Clock used by InterfaceRuntime.
     * @param quietMessageOption Runtime quiet-message option shown in panels.
     * @param displaySettings Display-owned settings shown/edited by panels.
     */
    CommandsInputRuntime(MillisecondClock& clock, Option& quietMessageOption,
        DisplayPresentationSettings& displaySettings);
    ~CommandsInputRuntime();

    /** @return Raw input event queue. */
    InputQueue& inputQueue();

    /** @return Command action registry. */
    CommandRegistry& commands();

    /** @return Command dispatcher. */
    CommandDispatcher& dispatcher();

    /** @return Keymap registry. */
    KeymapRegistry& keymaps();

    /** @return Active interface runtime. */
    InterfaceRuntime& interfaceRuntime();

    /** @return Display-facing error message source. */
    ErrorMessages& errorMessages();

    /** Applies startup input configuration to the input queue. */
    void configureInput(const InputConfig& config);

    /** Registers built-in command actions. */
    void registerBuiltins();

    /** Initializes keymaps from startup input configuration. */
    void initializeKeymaps(const InputConfig& config);

    /** Runs the currently selected interface against queued input. */
    void runCurrent(CommandContext& context);
};

#endif
