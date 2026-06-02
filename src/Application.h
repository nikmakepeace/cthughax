// Application lifecycle and shared graphical frame scheduler.

#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "PlatformLifecycle.h"
#include "VideoFilterchainSequence.h"

#include <memory>
#include <vector>

class AudioVisualBridge;
class IndexedFrame;
class Scene;
class SceneCommands;
class VideoFilterchain;

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
    PlatformLifecycle platformLifecycle;
    int shutdownComplete;

    static void platformWillSuspend(void* context);
    static void platformDidResume(void* context);
    void willSuspend();
    void didResume();
    void shutdownSceneRuntime();
    void shutdownVideoFilterchain();
    void shutdownAudioVisualBridge();
    void runAudioVisualBridge();
    const IndexedFrame* runVideoFilterchain();

public:
    Application(int argc, char* argv[]);
    ~Application();

    int initialize();
    void run();
    void initSceneRuntime();
    void initVideoFilterchain();
    void initAudioVisualBridge();
    void shutdown();
    void runFrame(int doDisplay);
    int exitStatus() const;

    Scene& scene();
    SceneCommands& sceneCommands();
};

#endif
