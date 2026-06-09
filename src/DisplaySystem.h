#ifndef __DISPLAY_SYSTEM_H
#define __DISPLAY_SYSTEM_H

#include "Configuration.h"
#include "DisplayRuntime.h"

#include <memory>
#include <string>
#include <vector>

class CthughaDisplay;
class ErrorMessages;
class FramePalette;
class FrameRenderContext;
class ImageOption;
class IndexedFrame;
class InterfaceRuntime;
class LogSink;
class RuntimeCommandSink;
class RuntimeCommandTargetRouter;
class RuntimeConfigRegistry;
class Scene;
class SceneVisualSelections;
class SecondsClock;

/** Dependencies required to open one display driver instance. */
class DisplayOpenRequest {
public:
    Scene& scene;
    ImageOption& images;
    SceneVisualSelections* sceneVisualSelections;
    RuntimeCommandSink& runtimeCommands;
    RuntimeCommandTargetRouter& runtimeCommandRouter;
    RuntimeConfigRegistry& runtimeConfigRegistry;
    const DisplayConfig& config;
    SecondsClock& clock;
    InterfaceRuntime& interfaceRuntime;
    ErrorMessages& errorMessages;
    LogSink& log;
    int* argc;
    char** argv;

    /**
     * Creates a driver-open request with borrowed application-owned services.
     *
     * The referenced objects must outlive the DisplaySystem that consumes the
     * returned driver components.
     */
    DisplayOpenRequest(Scene& scene_, ImageOption& images_,
        SceneVisualSelections* sceneVisualSelections_,
        RuntimeCommandSink& runtimeCommands_,
        RuntimeCommandTargetRouter& runtimeCommandRouter_,
        RuntimeConfigRegistry& runtimeConfigRegistry_,
        const DisplayConfig& config_, SecondsClock& clock_,
        InterfaceRuntime& interfaceRuntime_, ErrorMessages& errorMessages_,
        LogSink& log_, int* argc_, char** argv_);
};

/**
 * Owned display-driver components in shutdown dependency order.
 *
 * Members are declared device, backend, runtime, coordinator so destruction
 * happens coordinator first, then runtime, backend, and device.
 */
class DisplaySystemComponents {
    std::unique_ptr<DisplayDevice> deviceValue;
    std::unique_ptr<DisplayBackend> backendValue;
    std::unique_ptr<DisplayRuntime> runtimeValue;
    std::unique_ptr<CthughaDisplay> coordinatorValue;

public:
    /**
     * Takes ownership of all components created by a display driver factory.
     */
    DisplaySystemComponents(std::unique_ptr<DisplayDevice> device_,
        std::unique_ptr<DisplayBackend> backend_,
        std::unique_ptr<DisplayRuntime> runtime_,
        std::unique_ptr<CthughaDisplay> coordinator_);
    ~DisplaySystemComponents();

    /** @return Driver device implementation. */
    DisplayDevice& device();

    /** @return Driver backend implementation. */
    DisplayBackend& backend();

    /** @return Event and presentation runtime over the backend. */
    DisplayRuntime& runtime();

    /** @return Presentation coordinator, or NULL if a test factory omitted it. */
    CthughaDisplay* coordinator();
};

/** Factory for one compiled display driver implementation. */
class DisplayDriverFactory {
public:
    /** Destroys the factory interface. */
    virtual ~DisplayDriverFactory();

    /** @return Stable driver id handled by this factory. */
    virtual DisplayDriverId driverId() const = 0;

    /** @return Human-readable driver name used in diagnostics. */
    virtual const char* driverName() const = 0;

    /**
     * Opens the display driver and returns owned components.
     *
     * @return Components on success, NULL on driver startup failure.
     */
    virtual std::unique_ptr<DisplaySystemComponents> open(
        const DisplayOpenRequest& request) = 0;
};

/** Registry that selects among compiled display driver factories. */
class DisplayDriverRegistry {
    std::vector<DisplayDriverFactory*> factories;

public:
    /** Adds a compiled driver factory. The caller retains ownership. */
    void add(DisplayDriverFactory& factory);

    /** @return Factory for id, or NULL when that driver is unavailable. */
    DisplayDriverFactory* find(DisplayDriverId id) const;

    /**
     * Selects the configured driver.
     *
     * DisplayDriverAuto returns the first registered factory, preserving
     * deterministic startup priority.
     */
    DisplayDriverFactory* select(DisplayDriverId requested) const;
};

/** Display module root owning driver, runtime, and presentation coordinator. */
class DisplaySystem {
    std::unique_ptr<DisplaySystemComponents> componentsValue;
    std::string activeDriverNameValue;

public:
    /** Creates a closed display system. */
    DisplaySystem();
    ~DisplaySystem();

    /**
     * Opens the configured display driver through the registry.
     *
     * @return Zero on success, nonzero when no driver could be opened.
     */
    int open(DisplayDriverRegistry& registry, const DisplayOpenRequest& request);

    /** Closes the active display and releases driver resources. */
    void close();

    /** @return True after a driver has opened successfully. */
    bool isOpen() const;

    /** @return Active driver name, or an empty string when closed. */
    const char* activeDriverName() const;

    /** @return Active display device. */
    DisplayDevice& device();

    /** @return Active display runtime. */
    DisplayRuntime& runtime();

    /** @return Active presentation coordinator. */
    CthughaDisplay& coordinator();
};

#endif
