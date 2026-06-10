/** @file
 * Runtime command values for live option/configuration changes.
 */

#ifndef __RUNTIME_COMMAND_H
#define __RUNTIME_COMMAND_H

#include <string>

class RuntimeEffectControlTarget;
class RuntimeOptionTarget;

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
    RuntimeCommandActivateScene,
    RuntimeCommandToggleSceneLock,
    RuntimeCommandToggleSceneChoiceUse,
    RuntimeCommandChangeScreenBy,
    RuntimeCommandChangeScreenTo,
    RuntimeCommandChangeZoomBy,
    RuntimeCommandChangeZoomTo,
    RuntimeCommandChangeMaxFpsTo,
    RuntimeCommandChangeSoundProcessingBy,
    RuntimeCommandChangeSoundProcessingTo,
    RuntimeCommandChangeFireSensitivityTo,
    RuntimeCommandChangeFireSourceTo,
    RuntimeCommandToggleAutoChangeLock,
    RuntimeCommandChangeAutoChangeLockTo,
    RuntimeCommandChangeAutoChangeCumulativeFireLevelTo,
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
    RuntimeCommandToggleEffectChoiceUse,
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

/** Value object describing one runtime command. */
struct RuntimeCommand {
    RuntimeCommandType type;
    RuntimeSceneTarget sceneTarget;
    int value;
    const char* text;
    RuntimeEffectControlTarget* effectControlTarget;
    RuntimeOptionTarget* optionTarget;
    RuntimePaletteMetadataTarget* paletteMetadataTarget;

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
     * Creates an indexed scene-choice activation command.
     *
     * @param target Scene target containing the choice.
     * @param index Choice index to activate.
     * @return Runtime command.
     */
    static RuntimeCommand activateScene(RuntimeSceneTarget target, int index);

    /**
     * Creates a scene-selection lock toggle command.
     *
     * @param target Scene target whose lock should toggle.
     * @return Runtime command.
     */
    static RuntimeCommand toggleSceneLock(RuntimeSceneTarget target);

    /**
     * Creates a scene-choice use toggle command.
     *
     * @param target Scene target containing the choice.
     * @param index Choice index whose use flag should toggle.
     * @return Runtime command.
     */
    static RuntimeCommand toggleSceneChoiceUse(
        RuntimeSceneTarget target, int index);

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
     * Creates an absolute max-FPS change command.
     *
     * @param to Max frames per second; 0 disables pacing.
     * @return Runtime command.
     */
    static RuntimeCommand changeMaxFpsTo(int to);

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
     * Creates an absolute fire-sensitivity change command.
     *
     * @param sensitivity 0..100, where lower values suppress smaller bursts.
     * @return Runtime command.
     */
    static RuntimeCommand changeFireSensitivityTo(int sensitivity);

    /**
     * Creates an absolute fire-source change command.
     *
     * @param to Stable fire source name.
     * @return Runtime command.
     */
    static RuntimeCommand changeFireSourceTo(const char* to);

    /**
     * Creates an auto-change lock toggle command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand toggleAutoChangeLock();

    /**
     * Creates an absolute auto-change lock command.
     *
     * @param locked Nonzero to lock/disable automatic scene changes.
     * @return Runtime command.
     */
    static RuntimeCommand changeAutoChangeLockTo(int locked);

    /**
     * Creates an absolute cumulative-fire threshold command.
     *
     * @param threshold Fire accumulation threshold; 0 disables this trigger.
     * @return Runtime command.
     */
    static RuntimeCommand changeAutoChangeCumulativeFireLevelTo(
        int threshold);

    /**
     * Creates a command that persists the current runtime configuration.
     *
     * @return Runtime command.
     */
    static RuntimeCommand writeIni();

    /**
     * Creates a stop-and-continue persistence command.
     *
     * @return Runtime command.
     */
    static RuntimeCommand stopAndContinue();

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
     * Creates a relative effect-target change command.
     *
     * @param target Effect target to change.
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeEffectControlBy(
        RuntimeEffectControlTarget& target, int by);

    /**
     * Creates an absolute effect-target change command.
     *
     * @param target Effect target to change.
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeEffectControlTo(
        RuntimeEffectControlTarget& target, const char* to);

    /**
     * Creates an effect-target activation command.
     *
     * @param target Effect target to activate.
     * @param index Choice index to activate.
     * @return Runtime command.
     */
    static RuntimeCommand activateEffectControl(
        RuntimeEffectControlTarget& target, int index);

    /**
     * Creates an effect-target use toggle command.
     *
     * @param target Effect target containing the choice.
     * @param index Choice index whose use flag should toggle.
     * @return Runtime command.
     */
    static RuntimeCommand toggleEffectChoiceUse(
        RuntimeEffectControlTarget& target, int index);

    /**
     * Creates a relative runtime option-target change command.
     *
     * @param target Option target to change.
     * @param by Relative offset to apply.
     * @return Runtime command.
     */
    static RuntimeCommand changeOptionBy(RuntimeOptionTarget& target, int by);

    /**
     * Creates an absolute runtime option-target change command.
     *
     * @param target Option target to change.
     * @param to Choice text to select.
     * @return Runtime command.
     */
    static RuntimeCommand changeOptionTo(
        RuntimeOptionTarget& target, const char* to);

    /**
     * Creates an effect-target lock toggle command.
     *
     * @param target Effect target whose lock should toggle.
     * @return Runtime command.
     */
    static RuntimeCommand toggleEffectControlLock(
        RuntimeEffectControlTarget& target);

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
