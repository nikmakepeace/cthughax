/** @file
 * Maps JSON control protocol commands to RuntimeCommand values.
 */

#include "ControlCommandMapper.h"

#include "ControlProtocol.h"

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

namespace {

static void fail(std::string* code, std::string* message,
    const char* codeText, const char* messageText) {
    if (code != 0)
        *code = codeText;
    if (message != 0)
        *message = messageText;
}

static bool readStringMember(const ControlJsonValue& message,
    const char* name, std::string* value, std::string* errorCode,
    std::string* errorMessage) {
    const ControlJsonValue* member = message.member(name);
    if (member == 0 || member->type() != ControlJsonValue::StringType) {
        fail(errorCode, errorMessage, "bad-command", "missing string field");
        return false;
    }
    *value = member->asString();
    return true;
}

static bool readTextValue(const ControlJsonValue& message, std::string* value,
    std::string* errorCode, std::string* errorMessage) {
    const ControlJsonValue* member = message.member("value");
    if (member == 0) {
        fail(errorCode, errorMessage, "bad-command", "missing value");
        return false;
    }
    if (member->type() == ControlJsonValue::StringType) {
        *value = member->asString();
        return true;
    }
    if (member->type() == ControlJsonValue::NumberType) {
        char text[64];
        double valueNumber = member->asNumber();
        if (floor(valueNumber) == valueNumber)
            snprintf(text, sizeof(text), "%.0f", valueNumber);
        else
            snprintf(text, sizeof(text), "%.15g", valueNumber);
        *value = text;
        return true;
    }
    if (member->type() == ControlJsonValue::BoolType) {
        *value = member->asBool() ? "on" : "off";
        return true;
    }

    fail(errorCode, errorMessage, "bad-command", "unsupported value type");
    return false;
}

static bool readIntValue(const ControlJsonValue& message, int* value,
    std::string* errorCode, std::string* errorMessage) {
    const ControlJsonValue* member = message.member("value");
    if (member == 0) {
        fail(errorCode, errorMessage, "bad-command", "missing value");
        return false;
    }
    if (member->type() == ControlJsonValue::NumberType) {
        *value = int(member->asNumber());
        return true;
    }
    if (member->type() == ControlJsonValue::StringType) {
        char* end = 0;
        long parsed = strtol(member->asString().c_str(), &end, 10);
        if (end != member->asString().c_str() && *end == '\0') {
            *value = int(parsed);
            return true;
        }
    }

    fail(errorCode, errorMessage, "bad-command", "expected integer value");
    return false;
}

static bool readBoolValue(const ControlJsonValue& message, bool* value,
    std::string* errorCode, std::string* errorMessage) {
    const ControlJsonValue* member = message.member("value");
    if (member == 0) {
        fail(errorCode, errorMessage, "bad-command", "missing value");
        return false;
    }
    if (member->type() == ControlJsonValue::BoolType) {
        *value = member->asBool();
        return true;
    }
    if (member->type() == ControlJsonValue::NumberType) {
        *value = member->asNumber() != 0.0;
        return true;
    }
    if (member->type() == ControlJsonValue::StringType) {
        std::string text = member->asString();
        if (text == "on" || text == "yes" || text == "true"
            || text == "1") {
            *value = true;
            return true;
        }
        if (text == "off" || text == "no" || text == "false"
            || text == "0") {
            *value = false;
            return true;
        }
    }

    fail(errorCode, errorMessage, "bad-command", "expected boolean value");
    return false;
}

static bool sceneTargetForName(
    const std::string& name, RuntimeSceneTarget* target) {
    if (name == "scene.flame") {
        *target = RuntimeSceneFlame;
        return true;
    }
    if (name == "scene.translation") {
        *target = RuntimeSceneTranslation;
        return true;
    }
    if (name == "scene.image") {
        *target = RuntimeSceneImage;
        return true;
    }
    if (name == "scene.object") {
        *target = RuntimeSceneObject;
        return true;
    }
    if (name == "scene.table") {
        *target = RuntimeSceneTable;
        return true;
    }
    if (name == "scene.waveScale") {
        *target = RuntimeSceneWaveScale;
        return true;
    }
    if (name == "scene.palette") {
        *target = RuntimeScenePalette;
        return true;
    }
    if (name == "scene.flashlight") {
        *target = RuntimeSceneFlashlight;
        return true;
    }
    return false;
}

}

ControlMappedCommand::ControlMappedCommand()
    : command(RuntimeCommandChangeAll)
    , textStorage() { }

bool controlCommandFromJson(const ControlJsonValue& message,
    ControlMappedCommand* command, std::string* errorCode,
    std::string* errorMessage) {
    if (message.type() != ControlJsonValue::ObjectType) {
        fail(errorCode, errorMessage, "bad-command", "command must be object");
        return false;
    }

    const ControlJsonValue* version = message.member("v");
    if (version == 0 || int(version->asNumber(0)) != 1) {
        fail(errorCode, errorMessage, "bad-version", "unsupported protocol version");
        return false;
    }

    std::string op;
    std::string targetName;
    if (!readStringMember(message, "op", &op, errorCode, errorMessage)
        || !readStringMember(message, "target", &targetName, errorCode,
            errorMessage))
        return false;

    if (op != "set") {
        fail(errorCode, errorMessage, "bad-op", "unsupported operation");
        return false;
    }

    RuntimeSceneTarget sceneTarget = RuntimeSceneFlame;
    if (sceneTargetForName(targetName, &sceneTarget)) {
        if (!readTextValue(message, &command->textStorage, errorCode,
                errorMessage))
            return false;
        command->command = RuntimeCommand::changeSceneTo(
            sceneTarget, command->textStorage.c_str());
        return true;
    }

    if (targetName == "audio.processing") {
        if (!readTextValue(message, &command->textStorage, errorCode,
                errorMessage))
            return false;
        command->command = RuntimeCommand::changeSoundProcessingTo(
            command->textStorage.c_str());
        return true;
    }

    if (targetName == "audio.fireSensitivity") {
        int value = 0;
        if (!readIntValue(message, &value, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeFireSensitivityTo(value);
        return true;
    }

    if (targetName == "audio.fireSource") {
        if (!readTextValue(message, &command->textStorage, errorCode,
                errorMessage))
            return false;
        command->command = RuntimeCommand::changeFireSourceTo(
            command->textStorage.c_str());
        return true;
    }

    if (targetName == "display.maxFps") {
        int value = 0;
        if (!readIntValue(message, &value, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeMaxFpsTo(value);
        return true;
    }

    if (targetName == "display.screen") {
        if (!readTextValue(message, &command->textStorage, errorCode,
                errorMessage))
            return false;
        command->command = RuntimeCommand::changeScreenTo(
            command->textStorage.c_str());
        return true;
    }

    if (targetName == "autoChange.enabled") {
        bool enabled = false;
        if (!readBoolValue(message, &enabled, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeAutoChangeLockTo(
            enabled ? 0 : 1);
        return true;
    }

    if (targetName == "autoChange.cumulativeFireLevel") {
        int value = 0;
        if (!readIntValue(message, &value, errorCode, errorMessage))
            return false;
        command->command
            = RuntimeCommand::changeAutoChangeCumulativeFireLevelTo(value);
        return true;
    }

    fail(errorCode, errorMessage, "bad-target", "unsupported target");
    return false;
}
