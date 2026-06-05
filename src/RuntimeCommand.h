/** @file
 * Runtime command values for live option/configuration changes.
 */

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

/** Runtime command kinds understood by RuntimeCommandSink implementations. */
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

/** Interface for palette metadata commands that need a concrete target. */
class RuntimePaletteMetadataTarget {
public:
    /** Destroys the palette metadata target interface. */
    virtual ~RuntimePaletteMetadataTarget() { }

    /**
     * Saves palette metadata.
     *
     * @return Target-specific status value.
     */
    virtual int savePaletteMetadata() = 0;

    /** Reverts palette metadata after a failed or cancelled edit. */
    virtual void revertPaletteMetadata() = 0;
};

/** Runtime state snapshot written for stop-and-continue. */
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

    /** Creates an empty continuation state snapshot. */
    RuntimeContinuationState();
};

/** Value object describing one runtime command. */
struct RuntimeCommand {
    RuntimeCommandType type;
    RuntimeSceneTarget sceneTarget;
    int value;
    const char* text;
    EffectControl* effectControl;
    Option* option;
    RuntimePaletteMetadataTarget* paletteMetadataTarget;
    RuntimeContinuationState continuation;

    /**
     * Creates a command with default payload fields for a type.
     *
     * @param type_ Command type.
     */
    RuntimeCommand(RuntimeCommandType type_);

    /**
     * Creates a request-close command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand requestClose();

    /**
     * Creates a relative scene-option change command.
     *
     * @param target Scene target to change.
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeSceneBy(RuntimeSceneTarget target, int by);

    /**
     * Creates an absolute scene-option change command.
     *
     * @param target Scene target to change.
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeSceneTo(RuntimeSceneTarget target, const char* to);

    /**
     * Creates a relative screen/presentation change command.
     *
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeScreenBy(int by);

    /**
     * Creates an absolute screen/presentation change command.
     *
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeScreenTo(const char* to);

    /**
     * Creates a relative zoom change command.
     *
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeZoomBy(int by);

    /**
     * Creates an absolute zoom change command.
     *
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeZoomTo(const char* to);

    /**
     * Creates a relative sound-processing change command.
     *
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeSoundProcessingBy(int by);

    /**
     * Creates an absolute sound-processing change command.
     *
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeSoundProcessingTo(const char* to);

    /**
     * Creates an auto-change lock toggle command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand toggleAutoChangeLock();

    /**
     * Creates an audio-frame reset command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand resetAudioFrame();

    /**
     * Creates a command that persists the current runtime configuration.
     *
     * @return Runtime command.
     */
    static RuntimeCommand writeIni();

    /**
     * Creates a stop-and-continue persistence command.
     *
     * @param continuation Runtime continuation state to write.
     * @return Runtime command.
     */
    static RuntimeCommand stopAndContinue(
        const RuntimeContinuationState& continuation);

    /**
     * Creates a show-FPS toggle command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand toggleShowFps();

    /**
     * Creates a restore-current-scene command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand restoreScene();

    /**
     * Creates a save-preset command.
     *
     * @param slot Preset slot number.
     * @return Runtime command.
     */
    static RuntimeCommand savePreset(int slot);

    /**
     * Creates a restore-preset command.
     *
     * @param slot Preset slot number.
     * @return Runtime command.
     */
    static RuntimeCommand restorePreset(int slot);

    /**
     * Creates a command that changes all scene options.
     *
     * @return Runtime command.
     */
    static RuntimeCommand changeAll();

    /**
     * Creates a command that changes one scene option.
     *
     * @return Runtime command.
     */
    static RuntimeCommand changeOne();

    /**
     * Creates a command that selects a random palette.
     *
     * @return Runtime command.
     */
    static RuntimeCommand randomPalette();

    /**
     * Creates a command that adds a random palette.
     *
     * @return Runtime command.
     */
    static RuntimeCommand addRandomPalette();

    /**
     * Creates a relative EffectControl change command.
     *
     * @param option Effect control to change.
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeEffectControlBy(EffectControl& option, int by);

    /**
     * Creates an absolute EffectControl change command.
     *
     * @param option Effect control to change.
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeEffectControlTo(EffectControl& option, const char* to);

    /**
     * Creates an EffectControl activation command.
     *
     * @param option Effect control to activate.
     * @param index Choice index to activate.
     * @return Runtime command.
     */
    static RuntimeCommand activateEffectControl(EffectControl& option, int index);

    /**
     * Creates a relative generic Option change command.
     *
     * @param option Option to change.
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeOptionBy(Option& option, int by);

    /**
     * Creates an absolute generic Option change command.
     *
     * @param option Option to change.
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeOptionTo(Option& option, const char* to);

    /**
     * Creates an EffectControl lock toggle command.
     *
     * @param option Effect control whose lock should toggle.
     * @return Runtime command.
     */
    static RuntimeCommand toggleEffectControlLock(EffectControl& option);

    /**
     * Creates a palette metadata save command.
     *
     * @param target Palette metadata target that performs the save.
     * @return Runtime command.
     */
    static RuntimeCommand savePaletteMetadata(RuntimePaletteMetadataTarget& target);

    /**
     * Creates a palette metadata revert command.
     *
     * @param target Palette metadata target that performs the revert.
     * @return Runtime command.
     */
    static RuntimeCommand revertPaletteMetadata(RuntimePaletteMetadataTarget& target);
};

#endif
