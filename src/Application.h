// Application lifecycle and shared graphical frame scheduler.

#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "PlatformLifecycle.h"
#include "VideoFilterchainSequence.h"

#include <memory>
#include <vector>

class AudioVisualBridge;
class CthughaDisplay;
class DisplayRuntimeOwnership;
class IndexedFrame;
class Scene;
class SceneCommands;
class VideoFilterchain;

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
    int exitStatusValue;
    int ncursesInitialized;
    std::unique_ptr<VideoFilterchain> videoFilterchain;
    VideoFilterchainSequence videoFilterchainSequence;
    std::unique_ptr<AudioVisualBridge> audioVisualBridge;
    std::unique_ptr<Scene> sceneValue;
    std::unique_ptr<SceneCommands> sceneCommandsValue;
    std::unique_ptr<DisplayRuntimeOwnership> displayRuntimeOwnership;
    std::unique_ptr<CthughaDisplay> displayValue;
    PlatformLifecycle platformLifecycle;
    int shutdownComplete;

    /** PlatformLifecycle trampoline for willSuspend(). */
    static void platformWillSuspend(void* context);

    /** PlatformLifecycle trampoline for didResume(). */
    static void platformDidResume(void* context);

    /** Releases audio resources immediately before process suspension. */
    void willSuspend();

    /** Reopens audio resources after process resume. */
    void didResume();

    /** Destroys Scene/SceneCommands and disconnects legacy scene callbacks. */
    void shutdownSceneRuntime();

    /** Destroys the visual filterchain and its owned filters. */
    void shutdownVideoFilterchain();

    /** Destroys the audio-to-visual bridge and its AutoChanger state. */
    void shutdownAudioVisualBridge();

    /**
     * Runs the audio policy side of one visual frame.
     *
     * This updates audio analysis/acoustic context and refreshes visual filters
     * when an audio-side option change requires it.
     */
    void runAudioVisualBridge();

    /**
     * Runs the visual filterchain for one frame.
     *
     * @return Published IndexedFrame for display, or NULL when no frame is ready.
     */
    const IndexedFrame* runVideoFilterchain();

public:
    /**
     * Captures process arguments for later option and display initialization.
     *
     * @param argc Argument count from main().
     * @param argv Argument vector from main(); borrowed for process lifetime.
     */
    Application(int argc, char* argv[]);
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
     * Runs the main event/frame loop until cthugha_close is set.
     *
     * Each iteration processes display events, services the active interface,
     * runs one frame, and services the interface again for post-frame updates.
     */
    void run();

    /**
     * Initializes scene state and command facade.
     *
     * Must run before option parsing that can route changes through legacy
     * callbacks into SceneCommands.
     */
    void initSceneRuntime();

    /**
     * Creates the configured video filterchain and connects its frame palette
     * to the display device.
     */
    void initVideoFilterchain();

    /**
     * Creates the audio-to-visual bridge after scene commands are available.
     */
    void initAudioVisualBridge();

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

    /** @return Active SceneCommands facade. Valid after initSceneRuntime(). */
    SceneCommands& sceneCommands();
};

#endif
