/** @file
 * Application lifecycle, shutdown handling, and one-frame runtime dispatcher.
 */

#include "Application.h"

#include "cthugha.h"
#include "information.h"
#include "display.h"
#include "AudioFrame.h"
#include "AudioIngest.h"
#include "AudioSystem.h"
#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "AudioVisualBridge.h"
#include "AutoChanger.h"
#include "Border.h"
#include "EffectChoiceLoader.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "DisplayBackend.h"
#include "DisplayDevice.h"
#include "DisplayRuntime.h"
#include "EffectControlPolicy.h"
#include "Flashlight.h"
#include "FramePacer.h"
#include "IndexedFrame.h"
#include "Interface.h"
#include "IniFiles.h"
#include "Option.h"
#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeConfigRegistry.h"
#include "RuntimeChangeMediator.h"
#include "RuntimeDisplayControls.h"
#include "RuntimeEffectControls.h"
#include "RuntimePersistence.h"
#include "RuntimeShutdown.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "VideoFilterchain.h"
#include "VideoFilterchainFactory.h"
#include "VideoFilters.h"
#include "TranslationOptions.h"
#include "flames.h"
#include "imath.h"
#include "keymap.h"
#include "keys.h"
#include "waves.h"

#ifdef CTH_XWIN
#include "xcthugha.h"
#endif

#include <unistd.h>

static int initializeVisualCatalogs(const CthughaBuffer& buffer,
    const PathConfig& pathConfig);
static int loadEffectPolicyImages(const EffectPolicy& effectPolicy,
    const PathConfig& pathConfig);
static void emitStartupConfigDiagnostics(
    const std::vector<ConfigDiagnostic>& diagnostics);

static SystemFrameSleeper systemFrameSleeper;
static FramePacer framePacer(systemFrameSleeper);

static VideoFrameContext videoFrameContextFor(const AudioFrame& frame,
    const AcousticContext& acousticContext) {
    VideoFrameContext context;
    context.audioFrame = &frame;
    context.rawAudioData = frame.raw;
    context.processedWaveData = frame.processedWaveData;
    context.audioMetrics = &frame.metrics;
    context.acousticContext = &acousticContext;
    context.now = now;
    context.deltaT = deltaT;
    return context;
}

static void emitStartupConfigDiagnostics(
    const std::vector<ConfigDiagnostic>& diagnostics) {
    for (std::vector<ConfigDiagnostic>::const_iterator it = diagnostics.begin();
         it != diagnostics.end(); ++it) {
        if (it->severity == ConfigDiagnosticError) {
            CTH_ERROR("Configuration error from %s `%s': %s\n",
                it->source.c_str(), it->key.c_str(), it->message.c_str());
        } else if (it->severity == ConfigDiagnosticWarning) {
            CTH_WARN("Configuration warning from %s `%s': %s\n",
                it->source.c_str(), it->key.c_str(), it->message.c_str());
        } else {
            CTH_INFO("Configuration note from %s `%s': %s\n",
                it->source.c_str(), it->key.c_str(), it->message.c_str());
        }
    }
}

static int loadEffectPolicyImages(const EffectPolicy& effectPolicy,
    const PathConfig& pathConfig) {
    if (!effectPolicy.imageFilesEnabled)
        return 0;

    CTH_INFO("  loading image files...\n");
    CthughaBuffer& targetBuffer = CthughaBuffer::buffer;
    ImageOption& images = videoDirector().imageOption();
    int result = images.loadImages(pathConfig, targetBuffer.width(),
        targetBuffer.height());
    CTH_INFO("  number of loaded image files: %d\n", images.getNEntries());

    return result;
}

Application::Application(int argc, char* argv[])
    : argcValue(argc)
    , argvValue(argv)
    , displayArgv(argv, argv + argc)
    , exitStatusValue(1)
    , platformLifecycle(PlatformLifecycleCallbacks(
          &Application::platformWillSuspend, &Application::platformDidResume, this))
    , startupInitialized(0)
    , shutdownComplete(0) { }

Application::~Application() {
    shutdown();
}

void Application::platformWillSuspend(void* context) {
    ((Application*)context)->willSuspend();
}

void Application::platformDidResume(void* context) {
    ((Application*)context)->didResume();
}

void Application::willSuspend() {
    shutdownAudioIngest();
}

void Application::didResume() {
    if (initAudioIngest() && runtimeShutdownValue.get() != NULL)
        runtimeShutdownValue->requestClose();
}

bool Application::closeRequested() const {
    return (runtimeShutdownValue.get() != NULL)
        && runtimeShutdownValue->closeRequested();
}

void Application::initSceneRuntime() {
    if (sceneValue.get() != NULL)
        return;

    // SceneCommands is the modern target for legacy option callbacks, so create
    // it before full option parsing can trigger scene-changing work.
    sceneValue.reset(new Scene);
    videoDirector().bindScene(*sceneValue);
    sceneCommandsValue.reset(new SceneCommands(*sceneValue, CthughaBuffer::buffer,
        videoDirector().imageOption()));
    runtimeConfigRegistryValue.reset(new RuntimeConfigRegistry(startupConfigValue));
    runtimeConfigContributorValue.reset(
        new LegacyRuntimeConfigContributor(*sceneCommandsValue));
    runtimeConfigRegistryValue->addContributor(*runtimeConfigContributorValue);
    runtimePersistenceValue.reset(
        new IniRuntimePersistence(*runtimeConfigRegistryValue));
    runtimeShutdownValue.reset(new RuntimeCloseState());
    runtimeDisplayControlsValue.reset(new DefaultRuntimeDisplayControls());
    runtimeAudioControlsValue.reset(new DefaultRuntimeAudioControls());
    runtimeAutoChangeControlsValue.reset(new DefaultRuntimeAutoChangeControls());
    runtimeEffectControlsValue.reset(
        new DefaultRuntimeEffectControls());
    Interface::setRuntimeConfigRegistry(runtimeConfigRegistryValue.get());
    runtimeChangeMediatorValue.reset(new RuntimeChangeMediator(
        *sceneCommandsValue, *runtimePersistenceValue,
        *runtimeShutdownValue, *runtimeDisplayControlsValue,
        *runtimeAudioControlsValue, *runtimeAutoChangeControlsValue,
        *runtimeEffectControlsValue));
    Keymap::setRuntimeCommandSink(runtimeChangeMediatorValue.get());
    bindSceneCommandsForLegacyCallbacks(sceneCommandsValue.get());
}

void Application::shutdownSceneRuntime() {
    Keymap::setRuntimeCommandSink(NULL);
    bindSceneCommandsForLegacyCallbacks(NULL);
    videoDirector().unbindScene();
    Interface::setRuntimeConfigRegistry(NULL);
    runtimeChangeMediatorValue.reset();
    runtimeEffectControlsValue.reset();
    runtimeAutoChangeControlsValue.reset();
    runtimeAudioControlsValue.reset();
    runtimeDisplayControlsValue.reset();
    runtimePersistenceValue.reset();
    runtimeShutdownValue.reset();
    runtimeConfigRegistryValue.reset();
    runtimeConfigContributorValue.reset();
    sceneCommandsValue.reset();
    sceneValue.reset();
}

void Application::initVideoFilterchain() {
    if (videoFilterchain.get() != NULL)
        return;

    VideoFilterchainFactory factory;
    videoFilterchainSequence = videoDirector().defaultFilterchainSequence();
    videoFilterchain.reset(factory.create(videoFilterchainSequence));

    if (displayRuntimeOwnership.get() != NULL)
        displayRuntimeOwnership->device().setFramePalette(
            framePaletteFromFilterchain(*videoFilterchain));
}

void Application::shutdownVideoFilterchain() {
    videoFilterchain.reset();
}

int Application::initAudioIngest() {
    if (audioIngestValue.get() != NULL)
        return 0;

    audioIngestValue.reset(new AudioIngest(startupConfigValue.audio,
        CthughaBuffer::buffer.maxDimension()));
    if (audioIngestValue->start(1)) {
        audioIngestValue.reset();
        return 1;
    }

    return 0;
}

void Application::shutdownAudioIngest() {
    if (audioIngestValue.get() != NULL)
        audioIngestValue->stop();
    audioIngestValue.reset();
}

void Application::initAudioVisualBridge() {
    if (audioVisualBridge.get() == NULL) {
        audioVisualBridge.reset(new AudioVisualBridge(acousticContextValue,
            startupConfigValue.audioAnalysis.minNoise,
            runtimeChangeMediatorValue.get()));
        Interface::setAutoChangerStatusProvider(audioVisualBridge.get());
    }
}

void Application::shutdownAudioVisualBridge() {
    Interface::setAutoChangerStatusProvider(NULL);
    audioVisualBridge.reset();
}

void Application::runAudioVisualBridge(AudioFrame& frame) {
    initAudioVisualBridge();
    audioVisualBridge->runFrame(frame);

    // AutoChanger and audio processing can request option changes that require
    // filters to refresh cached scene/display state before visual mutation.
    if (audioVisualBridge->filterchainRefreshRequested()) {
        initVideoFilterchain();
        VideoFilterchainFactory factory;
        factory.refresh(*videoFilterchain, videoFilterchainSequence);
        audioVisualBridge->clearFilterchainRefreshRequest();
    }
}

const IndexedFrame* Application::runVideoFilterchain(AudioFrame& frame) {
    initVideoFilterchain();

    // The filterchain receives a snapshot-like context for this visual frame.
    // Audio frame data and frame-local metrics are owned by AudioIngest; filters
    // borrow them only during run().
    VideoFrameContext context = videoFrameContextFor(frame, acousticContextValue);

    CTH_TRACE("running filterchain=%p filters=%d\n", "video runtime",
        videoFilterchain.get(), videoFilterchain.get() ? videoFilterchain->size() : 0);
    CthughaBuffer* buffer = videoDirector().configureFilterchain(*videoFilterchain);
    if (buffer != NULL) {
        videoFilterchain->run(*buffer, context);
        return &videoFilterchain->indexedFrame();
    }

    return NULL;
}

void Application::shutdown() {
    if (shutdownComplete)
        return;

    shutdownComplete = 1;

    if (startupInitialized && startupConfigValue.app.optionsSaveEnabled
        && runtimePersistenceValue.get() != NULL)
        runtimePersistenceValue->writeCurrentConfig();

    shutdownAudioVisualBridge();
    displayValue.reset();
    cthughaDisplay = NULL;
    if (displayRuntimeOwnership.get() != NULL)
        displayRuntimeOwnership->shutdown();
    displayRuntimeOwnership.reset();
    platformLifecycle.shutdown();
    shutdownAudioIngest();
    shutdownVideoFilterchain();
    shutdownSceneRuntime();
}

Scene& Application::scene() {
    return *sceneValue;
}

SceneCommands& Application::sceneCommands() {
    return *sceneCommandsValue;
}

int Application::initialize() {
    srand(time(0));
    seteuid(getuid()); // give up root privileges

    ConfigBuildResult startupConfig = buildStartupConfig(argcValue, argvValue);
    startupConfigValue = startupConfig.config;
    startupConfigDiagnostics = startupConfig.diagnostics;
    cthugha_configure_logging(startupConfigValue.logging);

    emitStartupConfigDiagnostics(startupConfigDiagnostics);
    if (!startupConfig.ok()) {
        title();
        usage();
        return 0;
    }

    if (startupConfig.helpRequested) {
        title();
        usage();
        exitStatusValue = 0;
        return 0;
    }

    configureKeys(startupConfigValue.input);
    configureAudioOptions(startupConfigValue.audio);
    configureCthughaDisplay(startupConfigValue.display);
#ifdef CTH_XWIN
    configureDisplayDeviceX11(startupConfigValue.x11);
#endif
    configureAutoChanger(startupConfigValue.autoChange);
    configureEffectPolicy(startupConfigValue.effectPolicy);
    configureTranslationOptions(startupConfigValue.effectPolicy);
    configureWaveOptions(startupConfigValue.effectPolicy);
    configurePaletteOptions(startupConfigValue.effectPolicy);
    videoDirector().configureTransitions(startupConfigValue.sceneTransition);
    videoDirector().configureQuietMessages(startupConfigValue.messages);
    CthughaBuffer::buffer.setDimensions(startupConfigValue.display.bufferWidth,
        startupConfigValue.display.bufferHeight);

    remove_continuation_ini(startupConfigValue.paths);

    initSceneRuntime();

    title();
    videoDirector().silenceMessages().initialize();

    init_imath();

    CTH_INFO("Initializing the sound device...\n");
    if (initAudioIngest())
        return 0;

    // Visual catalogs depend on final buffer dimensions and must be available
    // before startup scene config can be matched to concrete catalog entries.
    CTH_INFO("Initializing cthugha Buffer...\n");
    if (initializeVisualCatalogs(CthughaBuffer::buffer, startupConfigValue.paths))
        return 0;
    CthughaBuffer::buffer.allocatePixels();
    if (loadEffectPolicyImages(startupConfigValue.effectPolicy,
            startupConfigValue.paths)) {
        exitStatusValue = 0;
        return 0;
    }
    init_border();
    init_flashlight();

    CTH_INFO("Setting initial effect controls...\n");
    sceneCommands().applyStartupConfig(startupConfigValue.scene);
    configureAudioProcessing(startupConfigValue.scene);

    // Interface/keymaps are available before display creation so early display
    // events and option panels can route input immediately.
    CTH_INFO("Initializing interface...\n");
    Interface::set("main");

    CTH_INFO("Initializing keymaps...\n");
    Keymap::init(startupConfigValue.input);

    CTH_INFO("Initializing display...\n");
    int displayArgc = int(displayArgv.size());
    if (cth_init(&displayArgc, displayArgv.data()))
        return 0;
    displayRuntimeOwnership = newDisplayDevice(scene(), sceneCommands(),
        *runtimeChangeMediatorValue,
        *runtimeConfigRegistryValue,
        startupConfigValue.display);
    if (displayRuntimeOwnership.get() == NULL)
        return 0;
    displayRuntimeOwnership->publishAliases();
    displayValue = newCthughaDisplay(displayRuntimeOwnership->device(),
        displayRuntimeOwnership->runtime());
    cthughaDisplay = displayValue.get();

    CTH_INFO("Initializing the audio-visual bridge...\n");
    initAudioVisualBridge();

    // Install platform hooks last; callbacks assume audio/display state exists.
    platformLifecycle.install();

    startupInitialized = 1;
    exitStatusValue = 0;
    return 1;
}

void Application::run() {
    // Main loop shape:
    //   1. collect platform/window input;
    //   2. let the active interface react before frame generation;
    //   3. generate and optionally present one frame;
    //   4. let the interface draw/react again after frame-side changes.
    while (!closeRequested()) {
        int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
        double loopStart = traceDisplayTiming ? getTime() : 0.0;
        double eventsStart = loopStart;
        double eventsEnd = loopStart;
        double preInterfaceStart = 0.0;
        double preInterfaceEnd = 0.0;
        double frameStart = 0.0;
        double frameEnd = 0.0;
        double postInterfaceStart = 0.0;
        double postInterfaceEnd = 0.0;
        double pacingStart = 0.0;
        double pacingEnd = 0.0;
        int frameWasRun = 0;
        double visualFrameStart = 0.0;

        DisplayEventStats eventStats
            = displayRuntimeOwnership->runtime().processEvents();
        if (traceDisplayTiming)
            eventsEnd = getTime();

        if (traceDisplayTiming)
            preInterfaceStart = getTime();
        Interface::current->run();
        if (traceDisplayTiming)
            preInterfaceEnd = getTime();

        if (!closeRequested()) {
            if (traceDisplayTiming)
                frameStart = getTime();
            runFrame(1);
            frameWasRun = 1;
            visualFrameStart = now;
            if (traceDisplayTiming)
                frameEnd = getTime();
        }

        if (traceDisplayTiming)
            postInterfaceStart = getTime();
        Interface::current->run();
        if (traceDisplayTiming)
            postInterfaceEnd = getTime();

        if (frameWasRun && !closeRequested()) {
            if (traceDisplayTiming)
                pacingStart = getTime();
            FramePacingResult pacing = framePacer.paceFrameEnd(visualFrameStart,
                getTime(), int(maxFramesPerSecond));
            if (traceDisplayTiming) {
                pacingEnd = getTime();
                CTH_TRACE("pacing target-ms=%.3f requested-sleep-ms=%.3f actual-pacing-ms=%.3f maxfps=%d\n",
                    "frame pacing",
                    (pacing.targetFrameEndSeconds - pacing.frameStartSeconds)
                        * 1000.0,
                    pacing.requestedSleepSeconds * 1000.0,
                    (pacingEnd - pacingStart) * 1000.0,
                    pacing.maxFramesPerSecond);
            }
        }

        if (traceDisplayTiming) {
            double loopEnd = getTime();
            CTH_TRACE("mainloop-ms=%.3f events-ms=%.3f pre-interface-ms=%.3f frame-ms=%.3f post-interface-ms=%.3f pacing-ms=%.3f events=%d resize-events=%d expose-events=%d\n",
                "display timing", (loopEnd - loopStart) * 1000.0,
                (eventsEnd - eventsStart) * 1000.0,
                (preInterfaceEnd - preInterfaceStart) * 1000.0,
                (frameEnd - frameStart) * 1000.0,
                (postInterfaceEnd - postInterfaceStart) * 1000.0,
                (pacingEnd - pacingStart) * 1000.0,
                eventStats.eventCount, eventStats.resizeEvents,
                eventStats.exposeEvents);
        }
    }

    CTH_INFO("Exiting cthugha...\n");
}

int Application::exitStatus() const {
    return exitStatusValue;
}

void Application::runFrame(int doDisplay) {
    double frameTiming[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int traceFrameTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    if (traceFrameTiming)
        frameTiming[0] = getTime();

    // Advance display timing first so now/deltaT describe this visual frame for
    // audio analysis, AutoChanger policy, and all visual filters.
    displayValue->nextFrame();
    if (traceFrameTiming)
        frameTiming[1] = getTime();

    audioIngestValue->tick();
    AudioFrame& audioFrame = audioIngestValue->currentFrame();
    if (audioIngestValue->complete() && runtimeShutdownValue.get() != NULL) {
        CTH_INFO("Stopping...\n");
        runtimeShutdownValue->requestClose();
    }
    if (traceFrameTiming)
        frameTiming[2] = getTime();

    // Analyze audio and run option-changing policy before visual filters read
    // SceneSettings.
    runAudioVisualBridge(audioFrame);
    if (traceFrameTiming)
        frameTiming[3] = getTime();

    // Mutate Cthugha's indexed active/passive buffers and publish a frame view.
    const IndexedFrame* indexedFrame = runVideoFilterchain(audioFrame);
    VideoFrameContext presentationContext = videoFrameContextFor(audioFrame,
        acousticContextValue);
    if (traceFrameTiming)
        frameTiming[4] = getTime();

    // Prefer the modern IndexedFrame path. Fall back to the legacy screen()
    // function path when no filterchain frame was published.
    double visualStart = getTime();
    if (doDisplay) {
        if (indexedFrame != NULL && indexedFrame->valid())
            displayValue->present(*indexedFrame, presentationContext);
        else
            displayValue->presentCurrent(presentationContext);
    }
    double visualEnd = getTime();
    if (traceFrameTiming)
        frameTiming[5] = visualEnd;
    if (displayValue.get() != NULL)
        displayValue->observeVisualLatency(visualEnd - visualStart);

    if (traceFrameTiming) {
        frameTiming[6] = getTime();
        CTH_TRACE("total-ms=%.3f next-frame=%.3f audio=%.3f bridge=%.3f buffer=%.3f display=%.3f do-display=%d\n",
            "frame timing",
            (frameTiming[6] - frameTiming[0]) * 1000.0,
            (frameTiming[1] - frameTiming[0]) * 1000.0,
            (frameTiming[2] - frameTiming[1]) * 1000.0,
            (frameTiming[3] - frameTiming[2]) * 1000.0,
            (frameTiming[4] - frameTiming[3]) * 1000.0,
            (frameTiming[5] - frameTiming[4]) * 1000.0,
            doDisplay);
    }

    // Service lifecycle requests only between frame stages.
    platformLifecycle.serviceFrameBoundary();
}

static int initializeVisualCatalogs(const CthughaBuffer& buffer,
    const PathConfig& pathConfig) {
    // Built-in visual choices and file-backed catalogs are application startup
    // state, not pixel-buffer state. They live here because option parsing can
    // change buffer dimensions and stage initial option names before startup.
    flame.add(_flames, _nFlames);

    if (init_flames())
        return 1;

    if (init_translate(buffer))
        return 1;

    if (init_wave(pathConfig))
        return 1;

    if (load_palettes(pathConfig))
        return 1;

    return 0;
}
