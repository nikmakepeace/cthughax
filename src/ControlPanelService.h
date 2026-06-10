/** @file
 * Optional runtime control-panel service boundary.
 */

#ifndef CTHUGHA_CONTROL_PANEL_SERVICE_H
#define CTHUGHA_CONTROL_PANEL_SERVICE_H

#include <memory>

class AudioProcessingSelector;
class DisplayPresentationSettings;
class LogSink;
class RuntimeCommandSink;
class RuntimeCommandTargetRouter;
class RuntimeConfigRegistry;
class SceneVisualSelections;

/** Borrowed runtime ports exposed to optional control-panel implementations. */
struct ControlPanelRuntimePorts {
    int processArgc;
    char** processArgv;
    SceneVisualSelections* sceneVisualSelections;
    RuntimeCommandSink* runtimeCommands;
    RuntimeCommandTargetRouter* runtimeCommandRouter;
    RuntimeConfigRegistry* runtimeConfigRegistry;
    DisplayPresentationSettings* displaySettings;
    AudioProcessingSelector* audioProcessingSelector;

    ControlPanelRuntimePorts()
        : processArgc(0)
        , processArgv(0)
        , sceneVisualSelections(0)
        , runtimeCommands(0)
        , runtimeCommandRouter(0)
        , runtimeConfigRegistry(0)
        , displaySettings(0)
        , audioProcessingSelector(0) { }
};

/** wx-free interface for optional control panels. */
class ControlPanelService {
public:
    virtual ~ControlPanelService() { }

    /**
     * Performs any host-toolkit setup that is safer before the display backend
     * starts. Implementations may leave the visible panel hidden.
     */
    virtual void prepare() = 0;

    /** Shows the panel when hidden, hides it when shown. */
    virtual void toggle() = 0;

    /** Hides the panel without destroying runtime-owned state. */
    virtual void hide() = 0;

    /** Services pending UI events from the application main loop. */
    virtual void pumpEvents() = 0;

    /** Refreshes visible controls from the current runtime state. */
    virtual void syncFromRuntime() = 0;
};

/** Creates a no-op service used when no panel implementation is linked. */
std::unique_ptr<ControlPanelService> createNullControlPanelService();

/** Creates the wxWidgets implementation when CTH_WX_PANEL is enabled. */
std::unique_ptr<ControlPanelService> createWxControlPanelService(
    const ControlPanelRuntimePorts& ports, LogSink& log);

#endif
