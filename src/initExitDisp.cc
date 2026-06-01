// Program entry point, shutdown handling, and one-frame runtime dispatcher.
// Most subsystems own their detailed setup; this file defines their startup
// order and the per-frame call sequence used by the display main loop.

#include "cthugha.h"
#include "cth_buffer.h"
#include "information.h"
#include "display.h"
#include "AudioSystem.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"
#include "AudioVisualBridge.h"
#include "AudioProcessor.h"
#include "Border.h"
#include "translate.h"
#include "options.h"
#include "keys.h"
#include "imath.h"
#include "waves.h"
#include "Option.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "CDPlayer.h"
#include "DisplayDevice.h"
#include "Flashlight.h"
#include "Interface.h"
#include "VideoFilters.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "VideoFilterchain.h"
#include "VideoFilterchainFactory.h"
#include "keymap.h"

#include <unistd.h>
#include <signal.h>

static VideoFilterchain* videoFilterchain = NULL;
static VideoFilterchainSequence videoFilterchainSequence;
static AudioVisualBridge* audioVisualBridge = NULL;
static Scene* scene = NULL;
static SceneCommands* sceneCommands = NULL;

static void initSceneRuntime() {
    if (scene != NULL)
        return;

    scene = new Scene;
    videoDirector().bindScene(*scene);
    sceneCommands = new SceneCommands(*scene, CthughaBuffer::buffer,
        videoDirector().imageOption());
    bindSceneCommandsForLegacyCallbacks(sceneCommands);
}

static void shutdownSceneRuntime() {
    bindSceneCommandsForLegacyCallbacks(NULL);
    videoDirector().unbindScene();
    delete sceneCommands;
    sceneCommands = NULL;
    delete scene;
    scene = NULL;
}

static void initVideoFilterchain() {
    if (videoFilterchain != NULL)
        return;

    VideoFilterchainFactory factory;
    videoFilterchainSequence = videoDirector().defaultFilterchainSequence();
    videoFilterchain = factory.create(videoFilterchainSequence);

    if (displayDevice != NULL)
        displayDevice->setFramePalette(framePaletteFromFilterchain(*videoFilterchain));
}

static void shutdownVideoFilterchain() {
    delete videoFilterchain;
    videoFilterchain = NULL;
}

static void initAudioVisualBridge() {
    if (audioVisualBridge == NULL)
        audioVisualBridge = new AudioVisualBridge(sceneCommands);
}

static void shutdownAudioVisualBridge() {
    delete audioVisualBridge;
    audioVisualBridge = NULL;
}

static void runAudioVisualBridge() {
    initAudioVisualBridge();
    audioVisualBridge->runFrame();

    if (audioVisualBridge->filterchainRefreshRequested()) {
        initVideoFilterchain();
        VideoFilterchainFactory factory;
        factory.refresh(*videoFilterchain, videoFilterchainSequence);
        audioVisualBridge->clearFilterchainRefreshRequest();
    }
}

static void runVideoFilterchain() {
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
        videoFilterchain, videoFilterchain ? videoFilterchain->size() : 0);
    CthughaBuffer* buffer = videoDirector().configureFilterchain(*videoFilterchain);
    if (buffer != NULL)
        videoFilterchain->run(*buffer, context);
}

void sig_tty_cont(int);
void sig_tty_stop(int) {
    CTH_INFO("Stopping...\n");

    signal(SIGCONT, sig_tty_cont);

    // Defer suspension until run() reaches a point outside graphics work.
    cthugha_pause = 1;
}
void sig_tty_cont(int) {
    CTH_INFO("Continuing...\n");

    init_sound(CthughaBuffer::buffer.maxDimension());

    signal(SIGTSTP, sig_tty_stop);

    raise(SIGCONT);
}

void deleter() {
    // AutoChanger owns final option persistence, so destroy the bridge first.
    shutdownAudioVisualBridge();
    delete cthughaDisplay;
    cthughaDisplay = NULL;
    delete displayDevice;
    displayDevice = NULL;
    audioRuntimeShutdown();
    delete cdPlayer;
    cdPlayer = NULL;
    shutdownVideoFilterchain();
    shutdownSceneRuntime();
}

int main(int argc, char* argv[]) {

    srand(time(0));
    seteuid(getuid()); // give up root privileges

    if (get_pre_params(argc, argv))
        return 1;

    initSceneRuntime();

    if (cth_init(&argc, argv))
        return 1;

    if (get_params(argc, argv))
        return 1;

    title();

    init_imath();

    atexit(deleter);

    if (ncurses_use) {
        init_ncurses();
        atexit(exit_ncurses);
    }

    CTH_INFO("Initializing the sound device...\n");
    init_sound(CthughaBuffer::buffer.maxDimension());

    CTH_INFO("Initializing CD player...\n");
    cdPlayer = new CDPlayer;

    CTH_INFO("Initializing cthugha Buffer...\n");
    CthughaBuffer::initAll();
    if (videoDirector().loadImages())
        exit(0);
    init_border();
    init_flashlight();

    CTH_INFO("Initializing display...\n");
    newDisplayDevice(*scene, *sceneCommands);
    newCthughaDisplay();

    CTH_INFO("Setting initial core options...\n");
    CoreOption::changeToInitial();
    audioProcessing.changeToInitial();
    sceneCommands->initializeFromOptions();

    CTH_INFO("Initializing interface...\n");
    Interface::set("main");

    CTH_INFO("Initializing keymaps...\n");
    Keymap::init();

    CTH_INFO("Initializing the audio-visual bridge...\n");
    initAudioVisualBridge();

    signal(SIGTSTP, sig_tty_stop);

    displayDevice->mainLoop();

    CTH_INFO("Exiting cthugha...\n");

    return 0;
}

void run(int doDisplay) {
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

    runVideoFilterchain();
    if (traceFrameTiming)
        frameTiming[4] = getTime();

    double visualStart = getTime();
    if (doDisplay)
        (*cthughaDisplay)();
    double visualEnd = getTime();
    if (traceFrameTiming)
        frameTiming[5] = visualEnd;
    if (cthughaDisplay)
        cthughaDisplay->observeVisualLatency(visualEnd - visualStart);

    (*cdPlayer)();
    if (traceFrameTiming) {
        frameTiming[6] = getTime();
        CTH_TRACE("total-ms=%.3f next-frame=%.3f audio=%.3f bridge=%.3f buffer=%.3f display=%.3f cd=%.3f do-display=%d\n",
            "frame timing",
            (frameTiming[6] - frameTiming[0]) * 1000.0,
            (frameTiming[1] - frameTiming[0]) * 1000.0,
            (frameTiming[2] - frameTiming[1]) * 1000.0,
            (frameTiming[3] - frameTiming[2]) * 1000.0,
            (frameTiming[4] - frameTiming[3]) * 1000.0,
            (frameTiming[5] - frameTiming[4]) * 1000.0,
            (frameTiming[6] - frameTiming[5]) * 1000.0,
            doDisplay);
    }

    // Suspend only between frame stages, after graphics operations are done.
    if (cthugha_pause) {
        cthugha_pause = 0;

        exit_sound();

        raise(SIGTSTP);
    }
}
