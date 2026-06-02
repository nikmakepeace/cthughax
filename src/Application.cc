// Application lifecycle, shutdown handling, and one-frame runtime dispatcher.

#include "Application.h"

#include "cthugha.h"
#include "information.h"
#include "display.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"
#include "AudioSystem.h"
#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "AudioVisualBridge.h"
#include "Border.h"
#include "CoreOption.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "Flashlight.h"
#include "IndexedFrame.h"
#include "Interface.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "VideoFilterchain.h"
#include "VideoFilterchainFactory.h"
#include "VideoFilters.h"
#include "imath.h"
#include "keymap.h"
#include "options.h"

#include <unistd.h>

static void configureTerminalTextMode();

Application::Application(int argc, char* argv[])
    : argcValue(argc)
    , argvValue(argv)
    , displayArgv(argv, argv + argc)
    , exitStatusValue(1)
    , ncursesInitialized(0)
    , platformLifecycle(PlatformLifecycleCallbacks(
          &Application::platformWillSuspend, &Application::platformDidResume, this))
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
    exit_sound();
}

void Application::didResume() {
    if (init_sound(CthughaBuffer::buffer.maxDimension()))
        cthugha_close++;
}

void Application::initSceneRuntime() {
    if (sceneValue.get() != NULL)
        return;

    sceneValue.reset(new Scene);
    videoDirector().bindScene(*sceneValue);
    sceneCommandsValue.reset(new SceneCommands(*sceneValue, CthughaBuffer::buffer,
        videoDirector().imageOption()));
    bindSceneCommandsForLegacyCallbacks(sceneCommandsValue.get());
}

void Application::shutdownSceneRuntime() {
    bindSceneCommandsForLegacyCallbacks(NULL);
    videoDirector().unbindScene();
    sceneCommandsValue.reset();
    sceneValue.reset();
}

void Application::initVideoFilterchain() {
    if (videoFilterchain.get() != NULL)
        return;

    VideoFilterchainFactory factory;
    videoFilterchainSequence = videoDirector().defaultFilterchainSequence();
    videoFilterchain.reset(factory.create(videoFilterchainSequence));

    if (displayDevice != NULL)
        displayDevice->setFramePalette(framePaletteFromFilterchain(*videoFilterchain));
}

void Application::shutdownVideoFilterchain() {
    videoFilterchain.reset();
}

void Application::initAudioVisualBridge() {
    if (audioVisualBridge.get() == NULL)
        audioVisualBridge.reset(new AudioVisualBridge(sceneCommandsValue.get()));
}

void Application::shutdownAudioVisualBridge() {
    audioVisualBridge.reset();
}

void Application::runAudioVisualBridge() {
    initAudioVisualBridge();
    audioVisualBridge->runFrame();

    if (audioVisualBridge->filterchainRefreshRequested()) {
        initVideoFilterchain();
        VideoFilterchainFactory factory;
        factory.refresh(*videoFilterchain, videoFilterchainSequence);
        audioVisualBridge->clearFilterchainRefreshRequest();
    }
}

const IndexedFrame* Application::runVideoFilterchain() {
    initVideoFilterchain();

    VideoFrameContext context;
    context.audioFrame = audioFrameCurrent();
    context.rawAudioData = audioFrameRawData();
    context.processedWaveData = audioFrameProcessedWaveData();
    context.audioMetrics = &audioMetrics;
    context.acousticContext = &acousticContext;
    context.now = now;
    context.deltaT = deltaT;

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

    // AutoChanger owns final option persistence, so destroy the bridge first.
    shutdownAudioVisualBridge();
    if (ncursesInitialized) {
        exit_ncurses();
        ncursesInitialized = 0;
    }
    delete cthughaDisplay;
    cthughaDisplay = NULL;
    delete displayDevice;
    displayDevice = NULL;
    platformLifecycle.shutdown();
    audioRuntimeShutdown();
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

    if (get_pre_params(argcValue, argvValue))
        return 0;

    if (params_request_help(argcValue, argvValue)) {
        title();
        usage();
        exitStatusValue = 0;
        return 0;
    }

    initSceneRuntime();

    if (get_params(argcValue, argvValue))
        return 0;

    title();

    init_imath();

    configureTerminalTextMode();
    if (ncurses_use) {
        if (init_ncurses())
            return 0;
        ncursesInitialized = 1;
    }

    CTH_INFO("Initializing the sound device...\n");
    if (init_sound(CthughaBuffer::buffer.maxDimension()))
        return 0;

    CTH_INFO("Initializing cthugha Buffer...\n");
    if (CthughaBuffer::initAll())
        return 0;
    if (videoDirector().loadImages()) {
        exitStatusValue = 0;
        return 0;
    }
    init_border();
    init_flashlight();

    CTH_INFO("Setting initial core options...\n");
    CoreOption::changeToInitial();
    audioProcessing.changeToInitial();
    sceneCommands().initializeFromOptions();

    CTH_INFO("Initializing interface...\n");
    Interface::set("main");

    CTH_INFO("Initializing keymaps...\n");
    Keymap::init();

    CTH_INFO("Initializing display...\n");
    int displayArgc = int(displayArgv.size());
    if (cth_init(&displayArgc, displayArgv.data()))
        return 0;
    if (newDisplayDevice(scene(), sceneCommands()))
        return 0;
    newCthughaDisplay();

    CTH_INFO("Initializing the audio-visual bridge...\n");
    initAudioVisualBridge();

    platformLifecycle.install();

    exitStatusValue = 0;
    return 1;
}

void Application::run() {
    while (cthugha_close == 0) {
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

        DisplayEventStats eventStats = displayDevice->processEvents();
        if (traceDisplayTiming)
            eventsEnd = getTime();

        if (traceDisplayTiming)
            preInterfaceStart = getTime();
        Interface::current->run();
        if (traceDisplayTiming)
            preInterfaceEnd = getTime();

        if (cthugha_close == 0) {
            if (traceDisplayTiming)
                frameStart = getTime();
            runFrame(1);
            if (traceDisplayTiming)
                frameEnd = getTime();
        }

        if (traceDisplayTiming)
            postInterfaceStart = getTime();
        Interface::current->run();
        if (traceDisplayTiming)
            postInterfaceEnd = getTime();

        if (traceDisplayTiming) {
            double loopEnd = getTime();
            CTH_TRACE("mainloop-ms=%.3f events-ms=%.3f pre-interface-ms=%.3f frame-ms=%.3f post-interface-ms=%.3f events=%d resize-events=%d expose-events=%d\n",
                "display timing", (loopEnd - loopStart) * 1000.0,
                (eventsEnd - eventsStart) * 1000.0,
                (preInterfaceEnd - preInterfaceStart) * 1000.0,
                (frameEnd - frameStart) * 1000.0,
                (postInterfaceEnd - postInterfaceStart) * 1000.0,
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

    cthughaDisplay->nextFrame();
    if (traceFrameTiming)
        frameTiming[1] = getTime();

    audioFrameTick();
    if (traceFrameTiming)
        frameTiming[2] = getTime();

    runAudioVisualBridge();
    if (traceFrameTiming)
        frameTiming[3] = getTime();

    const IndexedFrame* indexedFrame = runVideoFilterchain();
    if (traceFrameTiming)
        frameTiming[4] = getTime();

    double visualStart = getTime();
    if (doDisplay) {
        if (indexedFrame != NULL && indexedFrame->valid())
            cthughaDisplay->present(*indexedFrame);
        else
            (*cthughaDisplay)();
    }
    double visualEnd = getTime();
    if (traceFrameTiming)
        frameTiming[5] = visualEnd;
    if (cthughaDisplay)
        cthughaDisplay->observeVisualLatency(visualEnd - visualStart);

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

static void configureTerminalTextMode() {
#if HAVE_NCURSES == 1
    ncurses_use = DisplayDevice::text_on_term;
#else
    ncurses_use = 0;
    DisplayDevice::text_on_term = 0;
#endif
}
