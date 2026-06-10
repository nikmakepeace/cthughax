/** @file
 * Maps JSON control protocol commands to RuntimeCommand values.
 */

#ifndef CTHUGHA_CONTROL_COMMAND_MAPPER_H
#define CTHUGHA_CONTROL_COMMAND_MAPPER_H

#include "RuntimeCommand.h"

#include <string>

class ControlJsonValue;

struct ControlMappedCommand {
    RuntimeCommand command;
    std::string textStorage;

    ControlMappedCommand();
};

bool controlCommandFromJson(const ControlJsonValue& message,
    ControlMappedCommand* command, std::string* errorCode,
    std::string* errorMessage);

#endif
