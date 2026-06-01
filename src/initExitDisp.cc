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
#include "PipelineStageModules.h"
#include "Scene.h"
#include "VideoDirector.h"
#include "VideoPipeline.h"
#include "VideoPipelineFactory.h"
#include "keymap.h"

#include <unistd.h>
#include <signal.h>

static VideoPipeline* videoPipeline = NULL;
static VideoPipelineSequence videoPipelineSequence;
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

static void initVideoPipeline() {
    if (videoPipeline != NULL)
        return;

    VideoPipelineFactory factory;
    videoPipelineSequence = videoDirector().defaultPipelineSequence();
    videoPipeline = factory.create(videoPipelineSequence);

    if (displayDevice != NULL)
        displayDevice->setFramePalette(framePaletteFromPipeline(*videoPipeline));
}

static void shutdownVideoPipeline() {
    delete videoPipeline;
    videoPipeline = NULL;
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

    if (audioVisualBridge->pipelineRefreshRequested()) {
        initVideoPipeline();
        VideoPipelineFactory factory;
        factory.refresh(*videoPipeline, videoPipelineSequence);
        audioVisualBridge->clearPipelineRefreshRequest();
    }
}

static void runVideoPipeline() {
    initVideoPipeline();

    VideoFrameContext context;
    context.audioFrame = audioFrameCurrent();
    context.rawAudioData = audioFrameRawData();
    context.processedWaveData = audioFrameProcessedWaveData();
    context.audioMetrics = &audioMetrics;
    context.acousticContext = &acousticContext;
    context.now = now;
    context.deltaT = deltaT;

    CTH_TRACE("running pipeline=%p modules=%d\n", "visual runtime",
        videoPipeline, videoPipeline ? videoPipeline->size() : 0);
    CthughaBuffer* buffer = videoDirector().configurePipeline(*videoPipeline);
    if (buffer != NULL)
        videoPipeline->run(*buffer, context);
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
    audioRuntimeShutdown();
    delete cdPlayer;
    shutdownVideoPipeline();
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

// Ad hoc profiling of the main loop.
// This is not very exact, but it gives a good impression of where the time is spent.
// To enable it, define PROF in the Makefile and recompile.
//
// The timing is accumulated and output is printed every 25 frames
// It shows the time spent in each module (sound reading, sound analyzing, display, ...)
// and the total time for 25 frames.


#undef PROF
//#define PROF // uncomment to enable ad hoc profiling of the main loop


#ifdef PROF
#define PROFILING() T[profilingIndex++] = getTime()
#else
#define PROFILING()
#endif

void run(int doDisplay) {
#ifdef PROF
    double T[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int profilingIndex = 0;
#endif
    double frameTiming[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int traceFrameTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    if (traceFrameTiming)
        frameTiming[0] = getTime();

    PROFILING();
    cthughaDisplay->nextFrame();
    if (traceFrameTiming)
        frameTiming[1] = getTime();

    PROFILING();
    audioFrameTick();
    if (traceFrameTiming)
        frameTiming[2] = getTime();

    PROFILING();
    runAudioVisualBridge();
    if (traceFrameTiming)
        frameTiming[3] = getTime();

    PROFILING();
    runVideoPipeline();
    if (traceFrameTiming)
        frameTiming[4] = getTime();

    PROFILING();
    double visualStart = getTime();
    if (doDisplay)
        (*cthughaDisplay)();
    double visualEnd = getTime();
    if (traceFrameTiming)
        frameTiming[5] = visualEnd;
    if (cthughaDisplay)
        cthughaDisplay->observeVisualLatency(visualEnd - visualStart);

    PROFILING();
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

    PROFILING();
    // Suspend only between frame stages, after graphics operations are done.
    if (cthugha_pause) {
        cthugha_pause = 0;

        exit_sound();

        raise(SIGTSTP);
    }

    PROFILING();

#ifdef PROF
    static double Ts[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 8; i++)
        Ts[i] = Ts[i] + T[i + 1] - T[i];

    static int n = 24;
    n++;
    if (n == 25) {
        static double To = 0;
        double to = getTime();

        printf("%6.4f: ", to - To);
        To = to;

        for (int i = 0; i < 8; i++) {
            printf("%6.4f ", Ts[i]);
            Ts[i] = 0;
        }
        printf("\n");
        n = 0;
    }
#endif
}
