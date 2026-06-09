#include "CommandsInputRuntime.h"

#include "Configuration.h"
#include "DisplayPresentationOptions.h"
#include "InterfaceRuntime.h"
#include "ProcessServices.h"

CommandsInputRuntime::CommandsInputRuntime(MillisecondClock& clock,
    Option& quietMessageOption, DisplayPresentationSettings& displaySettings)
    : inputQueueValue()
    , commandsValue()
    , dispatcherValue()
    , keymapsValue()
    , interfaceRuntimeValue(new InterfaceRuntime(clock))
    , errorMessagesValue(new ErrorMessages()) {
    registerDefaultInterfaces(*interfaceRuntimeValue, quietMessageOption,
        displaySettings);
}

CommandsInputRuntime::~CommandsInputRuntime() {
}

InputQueue& CommandsInputRuntime::inputQueue() {
    return inputQueueValue;
}

CommandRegistry& CommandsInputRuntime::commands() {
    return commandsValue;
}

CommandDispatcher& CommandsInputRuntime::dispatcher() {
    return dispatcherValue;
}

KeymapRegistry& CommandsInputRuntime::keymaps() {
    return keymapsValue;
}

InterfaceRuntime& CommandsInputRuntime::interfaceRuntime() {
    return *interfaceRuntimeValue;
}

ErrorMessages& CommandsInputRuntime::errorMessages() {
    return *errorMessagesValue;
}

void CommandsInputRuntime::configureInput(const InputConfig& config) {
    inputQueueValue.configure(config);
}

void CommandsInputRuntime::registerBuiltins() {
    registerDefaultKeyActions(commandsValue);
    registerInterfaceKeyActions(commandsValue);
}

void CommandsInputRuntime::initializeKeymaps(const InputConfig& config) {
    keymapsValue.init(config, commandsValue);
}

void CommandsInputRuntime::runCurrent(CommandContext& context) {
    interfaceRuntimeValue->runCurrent(inputQueueValue, keymapsValue,
        commandsValue, dispatcherValue, context);
}
