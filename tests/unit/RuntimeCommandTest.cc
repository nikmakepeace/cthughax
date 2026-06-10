#include "RuntimeCommandSink.h"

#include <assert.h>
#include <string.h>

class RecordingSink : public RuntimeCommandSink {
public:
    int calls;
    RuntimeCommand lastCommand;
    RuntimeChangeSet response;

    RecordingSink()
        : calls(0)
        , lastCommand(RuntimeCommandChangeAll)
        , response() {
        response.sceneChanges = 1;
    }

    virtual RuntimeChangeSet apply(const RuntimeCommand& command) {
        calls++;
        lastCommand = command;
        return response;
    }
};

class DummyPaletteMetadataTarget : public RuntimePaletteMetadataTarget {
public:
    virtual int savePaletteMetadata() { return 1; }
    virtual void revertPaletteMetadata() { }
};

static void testRuntimeCommandFactoriesCaptureIntent() {
    RuntimeCommand sceneBy = RuntimeCommand::changeSceneBy(RuntimeSceneWave, 3);
    assert(sceneBy.type == RuntimeCommandChangeSceneBy);
    assert(sceneBy.sceneTarget == RuntimeSceneWave);
    assert(sceneBy.value == 3);

    RuntimeCommand sceneTo = RuntimeCommand::changeSceneTo(RuntimeScenePalette, "blue");
    assert(sceneTo.type == RuntimeCommandChangeSceneTo);
    assert(sceneTo.sceneTarget == RuntimeScenePalette);
    assert(strcmp(sceneTo.text, "blue") == 0);

    RuntimeCommand activateScene
        = RuntimeCommand::activateScene(RuntimeSceneImage, 4);
    assert(activateScene.type == RuntimeCommandActivateScene);
    assert(activateScene.sceneTarget == RuntimeSceneImage);
    assert(activateScene.value == 4);

    RuntimeCommand sceneLock
        = RuntimeCommand::toggleSceneLock(RuntimeSceneWave);
    assert(sceneLock.type == RuntimeCommandToggleSceneLock);
    assert(sceneLock.sceneTarget == RuntimeSceneWave);

    RuntimeCommand maxFps = RuntimeCommand::changeMaxFpsTo(60);
    assert(maxFps.type == RuntimeCommandChangeMaxFpsTo);
    assert(maxFps.value == 60);

    RuntimeCommand screen = RuntimeCommand::changeScreenTo("Source");
    assert(screen.type == RuntimeCommandChangeScreenTo);
    assert(strcmp(screen.text, "Source") == 0);

    RuntimeCommand autoChangeLock = RuntimeCommand::changeAutoChangeLockTo(1);
    assert(autoChangeLock.type == RuntimeCommandChangeAutoChangeLockTo);
    assert(autoChangeLock.value == 1);

    RuntimeCommand fireSensitivity
        = RuntimeCommand::changeFireSensitivityTo(37);
    assert(fireSensitivity.type == RuntimeCommandChangeFireSensitivityTo);
    assert(fireSensitivity.value == 37);

    RuntimeCommand fireSource
        = RuntimeCommand::changeFireSourceTo("low-pass-150hz-amplitude");
    assert(fireSource.type == RuntimeCommandChangeFireSourceTo);
    assert(strcmp(fireSource.text, "low-pass-150hz-amplitude") == 0);

    RuntimeCommand cumulativeFire
        = RuntimeCommand::changeAutoChangeCumulativeFireLevelTo(420);
    assert(cumulativeFire.type
        == RuntimeCommandChangeAutoChangeCumulativeFireLevelTo);
    assert(cumulativeFire.value == 420);

    RuntimeCommand sceneChoiceUse
        = RuntimeCommand::toggleSceneChoiceUse(RuntimeScenePalette, 2);
    assert(sceneChoiceUse.type == RuntimeCommandToggleSceneChoiceUse);
    assert(sceneChoiceUse.sceneTarget == RuntimeScenePalette);
    assert(sceneChoiceUse.value == 2);

    RuntimeCommand save = RuntimeCommand::savePreset(7);
    assert(save.type == RuntimeCommandSavePreset);
    assert(save.value == 7);

    RuntimeCommand close = RuntimeCommand::requestClose();
    assert(close.type == RuntimeCommandRequestClose);

    RuntimeCommand stop = RuntimeCommand::stopAndContinue();
    assert(stop.type == RuntimeCommandStopAndContinue);

    RuntimeCommand changeOne = RuntimeCommand::changeOne();
    assert(changeOne.type == RuntimeCommandChangeOne);

    RuntimeCommand changeAll = RuntimeCommand::changeAll();
    assert(changeAll.type == RuntimeCommandChangeAll);

    DummyPaletteMetadataTarget paletteTarget;
    RuntimeCommand savePalette
        = RuntimeCommand::savePaletteMetadata(paletteTarget);
    assert(savePalette.type == RuntimeCommandSavePaletteMetadata);
    assert(savePalette.paletteMetadataTarget == &paletteTarget);

    RuntimeCommand revertPalette
        = RuntimeCommand::revertPaletteMetadata(paletteTarget);
    assert(revertPalette.type == RuntimeCommandRevertPaletteMetadata);
    assert(revertPalette.paletteMetadataTarget == &paletteTarget);
}

static void testRuntimeChangeSetMergesFlags() {
    RuntimeChangeSet left;
    RuntimeChangeSet right;

    assert(!left.any());

    left.sceneChanges = 4;
    right.displayChanged = 1;
    right.audioProcessingChanged = 1;
    right.closeRequested = 1;
    right.uiChanged = 1;
    left.merge(right);

    assert(left.any());
    assert(left.sceneChanges == 4);
    assert(left.displayChanged == 1);
    assert(left.audioProcessingChanged == 1);
    assert(left.closeRequested == 1);
    assert(left.uiChanged == 1);
}

static void testRuntimeCommandSinkCapturesCommands() {
    RecordingSink sink;
    RuntimeChangeSet result = sink.apply(RuntimeCommand::toggleShowFps());

    assert(sink.calls == 1);
    assert(sink.lastCommand.type == RuntimeCommandToggleShowFps);
    assert(result.sceneChanges == 1);
}

int main() {
    testRuntimeCommandFactoriesCaptureIntent();
    testRuntimeChangeSetMergesFlags();
    testRuntimeCommandSinkCapturesCommands();
    return 0;
}
