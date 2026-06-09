#include "DisplaySystem.h"

#include "CthughaDisplay.h"
#include "ProcessServices.h"

#include <utility>

DisplayOpenRequest::DisplayOpenRequest(Scene& scene_, ImageOption& images_,
    SceneVisualSelections* sceneVisualSelections_,
    RuntimeCommandSink& runtimeCommands_,
    RuntimeCommandTargetRouter& runtimeCommandRouter_,
    RuntimeConfigRegistry& runtimeConfigRegistry_,
    const DisplayConfig& config_,
    DisplayPresentationSettings& presentationSettings_,
    SecondsClock& clock_, InterfaceRuntime& interfaceRuntime_,
    ErrorMessages& errorMessages_, LogSink& log_, int* argc_,
    char** argv_)
    : scene(scene_)
    , images(images_)
    , sceneVisualSelections(sceneVisualSelections_)
    , runtimeCommands(runtimeCommands_)
    , runtimeCommandRouter(runtimeCommandRouter_)
    , runtimeConfigRegistry(runtimeConfigRegistry_)
    , config(config_)
    , presentationSettings(presentationSettings_)
    , clock(clock_)
    , interfaceRuntime(interfaceRuntime_)
    , errorMessages(errorMessages_)
    , log(log_)
    , argc(argc_)
    , argv(argv_) {
}

DisplaySystemComponents::DisplaySystemComponents(
    std::unique_ptr<DisplayDevice> device_,
    std::unique_ptr<DisplayBackend> backend_,
    std::unique_ptr<DisplayRuntime> runtime_,
    std::unique_ptr<CthughaDisplay> coordinator_)
    : deviceValue(std::move(device_))
    , backendValue(std::move(backend_))
    , runtimeValue(std::move(runtime_))
    , coordinatorValue(std::move(coordinator_)) {
}

DisplaySystemComponents::~DisplaySystemComponents() {
}

DisplayDevice& DisplaySystemComponents::device() {
    return *deviceValue;
}

DisplayBackend& DisplaySystemComponents::backend() {
    return *backendValue;
}

DisplayRuntime& DisplaySystemComponents::runtime() {
    return *runtimeValue;
}

CthughaDisplay* DisplaySystemComponents::coordinator() {
    return coordinatorValue.get();
}

DisplayDriverFactory::~DisplayDriverFactory() {
}

void DisplayDriverRegistry::add(DisplayDriverFactory& factory) {
    factories.push_back(&factory);
}

DisplayDriverFactory* DisplayDriverRegistry::find(DisplayDriverId id) const {
    for (std::vector<DisplayDriverFactory*>::const_iterator it = factories.begin();
         it != factories.end(); ++it) {
        if ((*it)->driverId() == id)
            return *it;
    }

    return 0;
}

DisplayDriverFactory* DisplayDriverRegistry::select(
    DisplayDriverId requested) const {
    if (requested == DisplayDriverAuto) {
        if (factories.empty())
            return 0;
        return factories.front();
    }

    return find(requested);
}

DisplaySystem::DisplaySystem()
    : presentationSettingsValue()
    , componentsValue()
    , activeDriverNameValue() {
}

DisplaySystem::~DisplaySystem() {
    close();
}

int DisplaySystem::open(DisplayDriverRegistry& registry,
    const DisplayOpenRequest& request) {
    close();
    presentationSettingsValue.configure(request.config);

    DisplayDriverFactory* factory = registry.select(request.config.driver);
    if (factory == 0) {
        request.log.error("Display driver `%s' is not available.\n",
            displayDriverIdName(request.config.driver));
        return 1;
    }

    std::unique_ptr<DisplaySystemComponents> components = factory->open(request);
    if (components.get() == 0)
        return 1;

    activeDriverNameValue = factory->driverName();
    componentsValue = std::move(components);
    return 0;
}

void DisplaySystem::close() {
    componentsValue.reset();
    activeDriverNameValue.clear();
}

bool DisplaySystem::isOpen() const {
    return componentsValue.get() != 0;
}

const char* DisplaySystem::activeDriverName() const {
    return activeDriverNameValue.c_str();
}

DisplayPresentationSettings& DisplaySystem::settings() {
    return presentationSettingsValue;
}

DisplayDevice& DisplaySystem::device() {
    return componentsValue->device();
}

DisplayRuntime& DisplaySystem::runtime() {
    return componentsValue->runtime();
}

CthughaDisplay& DisplaySystem::coordinator() {
    return *componentsValue->coordinator();
}
