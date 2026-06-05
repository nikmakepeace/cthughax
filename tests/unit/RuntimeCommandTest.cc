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

    RuntimeCommand save = RuntimeCommand::savePreset(7);
    assert(save.type == RuntimeCommandSavePreset);
    assert(save.value == 7);

    RuntimeCommand close = RuntimeCommand::requestClose();
    assert(close.type == RuntimeCommandRequestClose);

    RuntimeContinuationState continuation;
    continuation.flame = "fire";
    continuation.presentation = "window";
    continuation.showFpsEnabled = 1;
    RuntimeCommand stop = RuntimeCommand::stopAndContinue(continuation);
    assert(stop.type == RuntimeCommandStopAndContinue);
    assert(stop.continuation.flame == "fire");
    assert(stop.continuation.presentation == "window");
    assert(stop.continuation.showFpsEnabled == 1);

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
