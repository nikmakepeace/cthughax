/** @file
 * Application lifecycle and shared graphical frame scheduler.
 */

#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "ApplicationDisplayFrontend.h"
#include "AudioAnalyzer.h"
#include "CommandsInputRuntime.h"
#include "PlatformLifecycle.h"
#include "Configuration.h"
#include "DisplaySystem.h"
#include "FrameGeneratorRuntime.h"
#include "FramePacer.h"
#include "ProcessServices.h"

#include <memory>
#include <vector>

class AudioFrame;
class AudioFramePipeline;
class AudioIngest;
class AudioProcessingSelector;
class AudioProcessingState;
class AudioProcessor;
class AutoChangeControls;
class AutoChangeQuietObserver;
class AutoChangeSettings;
class ErrorMessages;
class IndexedFrame;
class ImageOption;
class InterfaceRuntime;
class LegacyRuntimeConfigContributor;
class MixerControls;
class MixerDevice;
class MixerSession;
class OptionTime;
class RuntimeAudioControls;
class RuntimeAutoChangeControls;
class RuntimeChangeMediator;
class RuntimeConfigRegistry;
class RuntimeCommandTargetRouter;
class RuntimeDisplayControls;
class RuntimeEffectControls;
class RuntimePersistence;
class RuntimeShutdown;
class Scene;
class SceneChangeScheduler;
class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneRuntime;
class SceneSnapshot;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;
class SceneVisualCatalogFactory;

/**
 * Top-level graphical application lifecycle.
 *
 * Application owns startup sequencing, the display/audio/video runtime objects,
 * the main event/frame loop, platform suspend/resume hooks, and orderly
 * shutdown. main.cc intentionally does little more than construct this object,
 * call initialize(), run(), and return exitStatus().
 */
class Application {
    int argcValue;
    char** argvValue;
    std::vector<char*> displayArgv;
    SystemFrameSleeper frameSleeperValue;
    FramePacer framePacerValue;
    SystemMillisecondClock millisecondClockValue;
    SystemSecondsClock secondsClockValue;
    SystemCountdownTimerFactory countdownTimerFactoryValue;
    CStdRandomSource randomSourceValue;
    LoggingRuntime loggingRuntimeValue;
    ConsoleLogSink logSinkValue;
    std::unique_ptr<CommandsInputRuntime> commandsInputValue;
    int exitStatusValue;
    Config startupConfigValue;
    std::vector<ConfigDiagnostic> startupConfigDiagnostics;
    FrameGeneratorRuntime frameGeneratorValue;
    std::unique_ptr<ImageOption> imageOptionValue;
    std::unique_ptr<OptionTime> quietMessageOptionValue;
    LegacyDisplayFrontendInitializer displayFrontendInitializerValue;
    DisplayFrontendInitializer* displayFrontendInitializer;
    AcousticContext acousticContextValue;
    std::unique_ptr<AudioProcessor> audioProcessorValue;
    std::unique_ptr<AudioProcessingState> audioProcessingStateValue;
    std::unique_ptr<AudioProcessingSelector> audioProcessingSelectorValue;
    std::unique_ptr<AudioFramePipeline> audioFramePipelineValue;
    std::unique_ptr<AudioIngest> audioIngestValue;
    std::unique_ptr<AutoChangeSettings> autoChangeSettingsValue;
    std::unique_ptr<AutoChangeControls> autoChangeControlsValue;
    std::unique_ptr<AutoChangeQuietObserver> autoChangeQuietObserverValue;
    std::unique_ptr<SceneChangeScheduler> sceneChangeSchedulerValue;
    std::unique_ptr<SceneImageCatalog> sceneImageCatalogValue;
    std::unique_ptr<ScenePaletteCatalog> scenePaletteCatalogValue;
    std::unique_ptr<SceneTranslationCatalog> sceneTranslationCatalogValue;
    std::unique_ptr<SceneWaveObjectCatalog> sceneWaveObjectCatalogValue;
    std::unique_ptr<SceneVisualCatalogFactory> sceneVisualCatalogFactoryValue;
    std::unique_ptr<SceneRuntime> sceneRuntimeValue;
    std::unique_ptr<RuntimeConfigRegistry> runtimeConfigRegistryValue;
    std::unique_ptr<LegacyRuntimeConfigContributor> runtimeConfigContributorValue;
    std::unique_ptr<RuntimePersistence> runtimePersistenceValue;
    std::unique_ptr<RuntimeShutdown> runtimeShutdownValue;
    std::unique_ptr<RuntimeDisplayControls> runtimeDisplayControlsValue;
    std::unique_ptr<RuntimeAudioControls> runtimeAudioControlsValue;
    std::unique_ptr<RuntimeAutoChangeControls> runtimeAutoChangeControlsValue;
    std::unique_ptr<RuntimeEffectControls> runtimeEffectControlsValue;
    std::unique_ptr<RuntimeChangeMediator> runtimeChangeMediatorValue;
    std::unique_ptr<RuntimeCommandTargetRouter> runtimeCommandRouterValue;
    std::unique_ptr<MixerDevice> mixerDeviceValue;
    std::unique_ptr<MixerSession> mixerSessionValue;
    std::unique_ptr<MixerControls> mixerControlsValue;
    DisplaySystem displaySystemValue;
    PlatformLifecycle platformLifecycle;
    int startupInitialized;
    int shutdownComplete;

    /** PlatformLifecycle trampoline for willSuspend(). */
    static void platformWillSuspend(void* context);

    /** PlatformLifecycle trampoline for didResume(). */
    static void platformDidResume(void* context);

    /** Releases audio resources immediately before process suspension. */
    void willSuspend();

    /** Reopens audio resources after process resume. */
    void didResume();

    /** @return True when the application should leave the main loop. */
    bool closeRequested() const;

    /** Destroys scene-adjacent runtime command and persistence objects. */
    void shutdownSceneRuntime();

    /** Creates and installs OSS mixer controls for DSP input sessions. */
    int initMixerRuntime();

    /** Clears and releases the Application-owned mixer runtime. */
    void shutdownMixerRuntime();

    /** Destroys the frame generator filterchain and its owned filters. */
    void shutdownFrameGeneratorPipeline();

    /**
     * Creates and starts application-owned audio ingest.
     *
     * @return 0 on success, nonzero when requested startup audio cannot open.
     */
    int initAudioIngest();

    /** Stops audio ingest and releases its owned acquisition pipeline. */
    void shutdownAudioIngest();

    /** Destroys the audio frame pipeline and automatic scene-change state. */
    void shutdownAudioFramePipeline();

    /**
     * Runs the audio side of one visual frame.
     *
     * This updates audio analysis/acoustic context and runs automatic
     * scene-change policy from the resulting frame metrics.
     */
    void runAudioFramePipeline(AudioFrame& frame);

    /**
     * Runs the frame generator for one frame.
     *
     * @return Published IndexedFrame for display, or NULL when no frame is ready.
     */
    const IndexedFrame* runFrameGenerator(
        AudioFrame& frame, const SceneSnapshot& sceneSnapshot);

public:
    /**
     * Captures process arguments for later option and display initialization.
     *
     * @param argc Argument count from main().
     * @param argv Argument vector from main(); borrowed for process lifetime.
     */
    Application(int argc, char* argv[]);

    /**
     * Captures process arguments and an explicit display frontend initializer.
     *
     * @param argc Argument count from main().
     * @param argv Argument vector from main(); borrowed for process lifetime.
     * @param displayFrontendInitializer_ Display startup port used during
     *        initialize(). The referenced object must outlive Application.
     */
    Application(int argc, char* argv[],
        DisplayFrontendInitializer& displayFrontendInitializer_);

    /** Releases owned runtime objects through shutdown(). */
    ~Application();

    /**
     * Performs one-time startup before the main loop.
     *
     * Initializes options, scene state, terminal/display backends, audio, visual
     * buffers, image/palette/filter state, keymaps, and platform lifecycle hooks.
     *
     * @return Nonzero when run() should be entered; zero after help/failure.
     */
    int initialize();

    /**
     * Runs the main event/frame loop until shutdown is requested.
     *
     * Each iteration processes display events, services the active interface,
     * runs one frame, and services the interface again for post-frame updates.
     */
    void run();

    /**
     * Initializes scene state and command ports.
     *
     * Must run after visual catalog loading so Scene selections copy complete
     * catalog entries, and before startup scene config or runtime commands are
     * applied.
     */
    void initSceneRuntime();

    /**
     * Creates the configured frame generator filterchain and connects its
     * frame palette to the display device.
     */
    void initFrameGeneratorPipeline();

    /**
     * Creates the audio frame pipeline after scene commands are available.
     */
    void initAudioFramePipeline();

    /**
     * Idempotently tears down runtime objects in dependency order.
     */
    void shutdown();

    /**
     * Runs one visual frame.
     *
     * @param doDisplay Nonzero to present the generated frame. Zero runs timing,
     *        audio, bridge, and filterchain work without drawing to the display.
     */
    void runFrame(int doDisplay);

    /** @return Process exit status chosen by initialize()/runtime. */
    int exitStatus() const;

    /** @return Active Scene. Valid after initSceneRuntime(). */
    Scene& scene();

};

#endif
