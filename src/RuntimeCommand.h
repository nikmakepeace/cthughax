// Runtime command values for live option/configuration changes.

#ifndef __RUNTIME_COMMAND_H
#define __RUNTIME_COMMAND_H

#include <string>

class EffectControl;
class Option;

enum RuntimeSceneTarget {
    RuntimeSceneFlame,
    RuntimeSceneGeneralFlame,
    RuntimeSceneWave,
    RuntimeSceneWaveScale,
    RuntimeSceneObject,
    RuntimeSceneTranslation,
    RuntimeSceneBorder,
    RuntimeSceneFlashlight,
    RuntimeScenePalette,
    RuntimeSceneTable,
    RuntimeSceneImage
};

enum RuntimeCommandType {
    RuntimeCommandRequestClose,
    RuntimeCommandChangeSceneBy,
    RuntimeCommandChangeSceneTo,
    RuntimeCommandChangeScreenBy,
    RuntimeCommandChangeScreenTo,
    RuntimeCommandChangeZoomBy,
    RuntimeCommandChangeZoomTo,
    RuntimeCommandChangeSoundProcessingBy,
    RuntimeCommandChangeSoundProcessingTo,
    RuntimeCommandToggleAutoChangeLock,
    RuntimeCommandResetAudioFrame,
    RuntimeCommandWriteIni,
    RuntimeCommandStopAndContinue,
    RuntimeCommandToggleShowFps,
    RuntimeCommandRestoreScene,
    RuntimeCommandSavePreset,
    RuntimeCommandRestorePreset,
    RuntimeCommandChangeAll,
    RuntimeCommandChangeOne,
    RuntimeCommandRandomPalette,
    RuntimeCommandAddRandomPalette,
    RuntimeCommandChangeEffectControlBy,
    RuntimeCommandChangeEffectControlTo,
    RuntimeCommandActivateEffectControl,
    RuntimeCommandChangeOptionBy,
    RuntimeCommandChangeOptionTo,
    RuntimeCommandToggleEffectControlLock,
    RuntimeCommandSavePaletteMetadata,
    RuntimeCommandRevertPaletteMetadata
};

class RuntimePaletteMetadataTarget {
public:
    virtual ~RuntimePaletteMetadataTarget() { }
    virtual int savePaletteMetadata() = 0;
    virtual void revertPaletteMetadata() = 0;
};

struct RuntimeContinuationState {
    std::string flame;
    std::string generalFlame;
    std::string wave;
    std::string waveScale;
    std::string object;
    std::string translation;
    std::string palette;
    std::string border;
    std::string flashlight;
    std::string table;
    std::string image;
    std::string presentation;
    std::string audioProcessing;
    int showFpsEnabled;

    RuntimeContinuationState();
};

struct RuntimeCommand {
    RuntimeCommandType type;
    RuntimeSceneTarget sceneTarget;
    int value;
    const char* text;
    EffectControl* effectControl;
    Option* option;
    RuntimePaletteMetadataTarget* paletteMetadataTarget;
    RuntimeContinuationState continuation;

    RuntimeCommand(RuntimeCommandType type_);

    static RuntimeCommand requestClose();
    static RuntimeCommand changeSceneBy(RuntimeSceneTarget target, int by);
    static RuntimeCommand changeSceneTo(RuntimeSceneTarget target, const char* to);
    static RuntimeCommand changeScreenBy(int by);
    static RuntimeCommand changeScreenTo(const char* to);
    static RuntimeCommand changeZoomBy(int by);
    static RuntimeCommand changeZoomTo(const char* to);
    static RuntimeCommand changeSoundProcessingBy(int by);
    static RuntimeCommand changeSoundProcessingTo(const char* to);
    static RuntimeCommand toggleAutoChangeLock();
    static RuntimeCommand resetAudioFrame();
    static RuntimeCommand writeIni();
    static RuntimeCommand stopAndContinue(
        const RuntimeContinuationState& continuation);
    static RuntimeCommand toggleShowFps();
    static RuntimeCommand restoreScene();
    static RuntimeCommand savePreset(int slot);
    static RuntimeCommand restorePreset(int slot);
    static RuntimeCommand changeAll();
    static RuntimeCommand changeOne();
    static RuntimeCommand randomPalette();
    static RuntimeCommand addRandomPalette();
    static RuntimeCommand changeEffectControlBy(EffectControl& option, int by);
    static RuntimeCommand changeEffectControlTo(EffectControl& option, const char* to);
    static RuntimeCommand activateEffectControl(EffectControl& option, int index);
    static RuntimeCommand changeOptionBy(Option& option, int by);
    static RuntimeCommand changeOptionTo(Option& option, const char* to);
    static RuntimeCommand toggleEffectControlLock(EffectControl& option);
    static RuntimeCommand savePaletteMetadata(RuntimePaletteMetadataTarget& target);
    static RuntimeCommand revertPaletteMetadata(RuntimePaletteMetadataTarget& target);
};

#endif
