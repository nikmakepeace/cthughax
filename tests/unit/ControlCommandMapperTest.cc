/** @file
 * Unit coverage for JSON control command mapping.
 */

#include "ControlCommandMapper.h"
#include "ControlProtocol.h"

#include <assert.h>
#include <string.h>

static ControlJsonValue parseCommand(const char* text) {
    ControlJsonValue value;
    std::string error;
    assert(parseControlJson(text, &value, &error));
    return value;
}

static void testMapsSceneSetCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"id\":12,\"op\":\"set\",\"target\":\"scene.flame\","
        "\"value\":\"fire\"}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeSceneTo);
    assert(mapped.command.sceneTarget == RuntimeSceneFlame);
    assert(strcmp(mapped.command.text, "fire") == 0);
    assert(mapped.textStorage == "fire");
}

static void testMapsAudioProcessingCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"audio.processing\","
        "\"value\":\"FFT\"}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeSoundProcessingTo);
    assert(strcmp(mapped.command.text, "FFT") == 0);
}

static void testMapsFireSensitivityCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"audio.fireSensitivity\","
        "\"value\":37}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeFireSensitivityTo);
    assert(mapped.command.value == 37);
}

static void testMapsFireSourceCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"audio.fireSource\","
        "\"value\":\"low-pass-150hz-amplitude\"}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeFireSourceTo);
    assert(strcmp(mapped.command.text, "low-pass-150hz-amplitude") == 0);
    assert(mapped.textStorage == "low-pass-150hz-amplitude");
}

static void testMapsMaxFpsCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"display.maxFps\","
        "\"value\":72}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeMaxFpsTo);
    assert(mapped.command.value == 72);
}

static void testMapsScreenCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"display.screen\","
        "\"value\":\"Source\"}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeScreenTo);
    assert(strcmp(mapped.command.text, "Source") == 0);
}

static void testMapsAutoChangeEnabledCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"autoChange.enabled\","
        "\"value\":false}"),
        &mapped, &code, &message));

    assert(mapped.command.type == RuntimeCommandChangeAutoChangeLockTo);
    assert(mapped.command.value == 1);
}

static void testMapsCumulativeFireThresholdCommand() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\","
        "\"target\":\"autoChange.cumulativeFireLevel\",\"value\":420}"),
        &mapped, &code, &message));

    assert(mapped.command.type
        == RuntimeCommandChangeAutoChangeCumulativeFireLevelTo);
    assert(mapped.command.value == 420);
}

static void testRejectsUnknownTargets() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(!controlCommandFromJson(parseCommand(
        "{\"v\":1,\"op\":\"set\",\"target\":\"scene.nope\","
        "\"value\":\"x\"}"),
        &mapped, &code, &message));

    assert(code == "bad-target");
    assert(!message.empty());
}

static void testRejectsUnsupportedVersion() {
    ControlMappedCommand mapped;
    std::string code;
    std::string message;

    assert(!controlCommandFromJson(parseCommand(
        "{\"v\":2,\"op\":\"set\",\"target\":\"scene.flame\","
        "\"value\":\"fire\"}"),
        &mapped, &code, &message));

    assert(code == "bad-version");
}

int main() {
    testMapsSceneSetCommand();
    testMapsAudioProcessingCommand();
    testMapsFireSensitivityCommand();
    testMapsFireSourceCommand();
    testMapsMaxFpsCommand();
    testMapsScreenCommand();
    testMapsAutoChangeEnabledCommand();
    testMapsCumulativeFireThresholdCommand();
    testRejectsUnknownTargets();
    testRejectsUnsupportedVersion();
    return 0;
}
