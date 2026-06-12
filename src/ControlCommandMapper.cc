/** @file
 * Maps JSON control protocol commands to RuntimeCommand values.
 */

#include "ControlCommandMapper.h"

#include "ControlProtocol.h"

#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <vector>

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

static bool readDoubleValue(const ControlJsonValue& message, double* value,
    std::string* errorCode, std::string* errorMessage) {
    const ControlJsonValue* member = message.member("value");
    if (member == 0) {
        fail(errorCode, errorMessage, "bad-command", "missing value");
        return false;
    }
    if (member->type() == ControlJsonValue::NumberType) {
        *value = member->asNumber();
        return true;
    }
    if (member->type() == ControlJsonValue::StringType) {
        char* end = 0;
        double parsed = strtod(member->asString().c_str(), &end);
        if (end != member->asString().c_str() && *end == '\0') {
            *value = parsed;
            return true;
        }
    }

    fail(errorCode, errorMessage, "bad-command", "expected numeric value");
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

static bool readBoolObjectMember(const ControlJsonValue& object,
    const char* name, bool fallback, bool* value, std::string* errorCode,
    std::string* errorMessage) {
    const ControlJsonValue* member = object.member(name);
    if (member == 0) {
        *value = fallback;
        return true;
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

    fail(errorCode, errorMessage, "bad-command", "expected boolean stage field");
    return false;
}

static bool readFilterchainSequenceValue(const ControlJsonValue& message,
    std::vector<std::string>* stages, std::vector<int>* enabled,
    std::string* errorCode, std::string* errorMessage) {
    const ControlJsonValue* member = message.member("value");
    if (member == 0) {
        fail(errorCode, errorMessage, "bad-command", "missing value");
        return false;
    }
    if (member->type() != ControlJsonValue::ArrayType) {
        fail(errorCode, errorMessage, "bad-command",
            "expected filterchain array value");
        return false;
    }

    stages->clear();
    enabled->clear();
    const std::vector<ControlJsonValue>& array = member->asArray();
    for (std::vector<ControlJsonValue>::const_iterator it = array.begin();
         it != array.end(); ++it) {
        if (it->type() == ControlJsonValue::StringType) {
            stages->push_back(it->asString());
            enabled->push_back(1);
            continue;
        }
        if (it->type() == ControlJsonValue::ObjectType) {
            const ControlJsonValue* stage = it->member("stage");
            if (stage == 0)
                stage = it->member("name");
            if (stage == 0 || stage->type() != ControlJsonValue::StringType) {
                fail(errorCode, errorMessage, "bad-command",
                    "expected filterchain stage name");
                return false;
            }

            bool stageEnabled = true;
            if (!readBoolObjectMember(*it, "enabled", true, &stageEnabled,
                    errorCode, errorMessage))
                return false;

            stages->push_back(stage->asString());
            enabled->push_back(stageEnabled ? 1 : 0);
            continue;
        }

        fail(errorCode, errorMessage, "bad-command",
            "expected filterchain stage value");
        return false;
    }
    return true;
}

static bool filterchainStageNameSupported(const std::string& name) {
    return name == "image" || name == "border" || name == "flame"
        || name == "translate" || name == "wave" || name == "text"
        || name == "flashlight" || name == "frameCommit"
        || name == "palette" || name == "indexedFrame";
}

static bool validateFilterchainSequenceNames(
    const std::vector<std::string>& stages, std::string* errorCode,
    std::string* errorMessage) {
    for (std::vector<std::string>::const_iterator it = stages.begin();
         it != stages.end(); ++it) {
        if (!filterchainStageNameSupported(*it)) {
            fail(errorCode, errorMessage, "bad-command",
                "unsupported filterchain stage");
            return false;
        }
    }
    return true;
}

static bool sceneTargetForName(
    const std::string& name, RuntimeSceneTarget* target) {
    if (name == "scene.flame") {
        *target = RuntimeSceneFlame;
        return true;
    }
    if (name == "scene.wave") {
        *target = RuntimeSceneWave;
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

static bool sceneLockTargetForName(
    const std::string& name, RuntimeSceneTarget* target) {
    const char suffix[] = ".locked";
    size_t suffixLength = sizeof(suffix) - 1;
    if (name.size() <= suffixLength)
        return false;
    if (name.compare(name.size() - suffixLength, suffixLength, suffix) != 0)
        return false;

    return sceneTargetForName(
        name.substr(0, name.size() - suffixLength), target);
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
    if (sceneLockTargetForName(targetName, &sceneTarget)) {
        bool locked = false;
        if (!readBoolValue(message, &locked, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeSceneLockTo(
            sceneTarget, locked ? 1 : 0);
        return true;
    }

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

    if (targetName == "display.screen.locked") {
        bool locked = false;
        if (!readBoolValue(message, &locked, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeScreenLockTo(
            locked ? 1 : 0);
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

    if (targetName == "audio.processing.locked") {
        bool locked = false;
        if (!readBoolValue(message, &locked, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeSoundProcessingLockTo(
            locked ? 1 : 0);
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

    if (targetName == "autoChange.changeLittle") {
        bool enabled = false;
        if (!readBoolValue(message, &enabled, errorCode, errorMessage))
            return false;
        command->command = RuntimeCommand::changeAutoChangeChangeLittleTo(
            enabled ? 1 : 0);
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

    if (targetName == "sceneTransition.paletteSmoothingChance") {
        double value = 0.0;
        if (!readDoubleValue(message, &value, errorCode, errorMessage))
            return false;
        command->command
            = RuntimeCommand::changePaletteSmoothingChanceTo(value);
        return true;
    }

    if (targetName == "filterchain.sequence") {
        std::vector<std::string> stages;
        std::vector<int> enabled;
        if (!readFilterchainSequenceValue(message, &stages, &enabled,
                errorCode, errorMessage)
            || !validateFilterchainSequenceNames(
                stages, errorCode, errorMessage))
            return false;
        command->command
            = RuntimeCommand::changeFilterchainSequenceTo(stages, enabled);
        return true;
    }

    if (targetName == "filterchain.enabled") {
        std::vector<std::string> stages;
        std::vector<int> enabled;
        if (!readFilterchainSequenceValue(message, &stages, &enabled,
                errorCode, errorMessage)
            || !validateFilterchainSequenceNames(
                stages, errorCode, errorMessage))
            return false;
        command->command
            = RuntimeCommand::changeFilterchainEnabledTo(stages, enabled);
        return true;
    }

    fail(errorCode, errorMessage, "bad-target", "unsupported target");
    return false;
}
