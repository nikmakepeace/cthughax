#include "CthughaDisplay.h"
#include "DisplayPresentationOptions.h"
#include "DisplaySystem.h"
#include "InputQueue.h"
#include "ProcessServices.h"

#include <assert.h>
#include <memory>
#include <stdarg.h>
#include <vector>

static std::vector<int> destroyedObjects;

int DisplayDevice::textColorRGB[][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
int DisplayDevice::textColor[3] = { 0, 0, 0 };
int DisplayDevice::textColors = 3;

DisplayDevice::DisplayDevice()
    : textOnScreen(0)
    , darkenPalette(0)
    , needsFullCopy(0)
    , framePalette(0) {
}

DisplayDevice::~DisplayDevice() {
}

void DisplayDevice::setFramePalette(FramePalette* framePalette_) {
    framePalette = framePalette_;
}

int DisplayDevice::setGlobalPalette() {
    return 0;
}

void DisplayDevice::prePrint() {
}

double DisplayDevice::print(const char*, double, int, int, int) {
    return 0.0;
}

void DisplayDevice::printString(int, int, const char*, int, int, int) {
}

void DisplayDevice::postPrint() {
}

int screen_up(ScreenRenderContext&) { return 0; }
int screen_down(ScreenRenderContext&) { return 0; }
int screen_2hor(ScreenRenderContext&) { return 0; }
int screen_r2hor(ScreenRenderContext&) { return 0; }
int screen_4hor(ScreenRenderContext&) { return 0; }
int screen_2verd(ScreenRenderContext&) { return 0; }
int screen_r2verd(ScreenRenderContext&) { return 0; }
int screen_4kal(ScreenRenderContext&) { return 0; }
int screen_hfield(ScreenRenderContext&) { return 0; }
int screen_roll(ScreenRenderContext&) { return 0; }
int screen_zick(ScreenRenderContext&) { return 0; }
int screen_bent(ScreenRenderContext&) { return 0; }
int screen_plate(ScreenRenderContext&) { return 0; }
int screen_vscale_hmirror(ScreenRenderContext&) { return 0; }
int screen_hscale_vmirror(ScreenRenderContext&) { return 0; }
int screen_source(ScreenRenderContext&) { return 0; }

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_context(int, const char*, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

int cth_log_errno(int, const char*, ...) {
    return 0;
}

class FakeSecondsClock : public SecondsClock {
public:
    virtual double nowSeconds() const {
        return 0.0;
    }
};

static FakeSecondsClock clockValue;
static DisplayPresentationSettings settingsValue;

class RecordingDevice : public DisplayDevice {
    int id;

public:
    explicit RecordingDevice(int id_)
        : DisplayDevice()
        , id(id_) {
    }

    virtual ~RecordingDevice() {
        destroyedObjects.push_back(id);
    }
};

class RecordingBackend : public DisplayBackend {
    int id;

public:
    explicit RecordingBackend(int id_)
        : id(id_) {
    }

    virtual ~RecordingBackend() {
        destroyedObjects.push_back(id);
    }

    virtual DisplayEventStats processEvents(InputEventSink&) {
        return DisplayEventStats();
    }

    virtual PixelSize outputSize() const {
        return PixelSize(1, 1);
    }

    virtual void present(const DisplayPresentation&) {
    }
};

class RecordingCoordinator : public CthughaDisplay {
    int id;

public:
    RecordingCoordinator(int id_, DisplayDevice& device,
        DisplayRuntime& runtime)
        : CthughaDisplay(device, runtime, clockValue, settingsValue)
        , id(id_) {
    }

    virtual ~RecordingCoordinator() {
        destroyedObjects.push_back(id);
    }
};

class NamedFactory : public DisplayDriverFactory {
    DisplayDriverId idValue;
    const char* nameValue;

public:
    NamedFactory(DisplayDriverId id_, const char* name_)
        : idValue(id_)
        , nameValue(name_) {
    }

    virtual DisplayDriverId driverId() const {
        return idValue;
    }

    virtual const char* driverName() const {
        return nameValue;
    }

    virtual std::unique_ptr<DisplaySystemComponents> open(
        const DisplayOpenRequest&) {
        return std::unique_ptr<DisplaySystemComponents>();
    }
};

static void testComponentsDestroyCoordinatorBeforeBackendAndDevice() {
    destroyedObjects.clear();

    std::unique_ptr<RecordingDevice> device(new RecordingDevice(1));
    RecordingDevice* deviceAlias = device.get();
    std::unique_ptr<RecordingBackend> backend(new RecordingBackend(2));
    RecordingBackend* backendAlias = backend.get();
    std::unique_ptr<DisplayRuntime> runtime(new DisplayRuntime(*backend));
    DisplayRuntime* runtimeAlias = runtime.get();
    std::unique_ptr<CthughaDisplay> coordinator(
        new RecordingCoordinator(3, *device, *runtime));
    CthughaDisplay* coordinatorAlias = coordinator.get();

    {
        DisplaySystemComponents components(std::move(device),
            std::move(backend), std::move(runtime), std::move(coordinator));

        assert(&components.device() == deviceAlias);
        assert(&components.backend() == backendAlias);
        assert(&components.runtime() == runtimeAlias);
        assert(components.coordinator() == coordinatorAlias);
    }

    assert(destroyedObjects.size() == 3);
    assert(destroyedObjects[0] == 3);
    assert(destroyedObjects[1] == 2);
    assert(destroyedObjects[2] == 1);
}

static void testRegistrySelectsAutoByRegistrationOrderAndExplicitDriver() {
    NamedFactory x11(DisplayDriverX11, "x11");
    NamedFactory sdl3(DisplayDriverSDL3, "sdl3");
    DisplayDriverRegistry registry;

    registry.add(x11);
    registry.add(sdl3);

    assert(registry.select(DisplayDriverAuto) == &x11);
    assert(registry.select(DisplayDriverX11) == &x11);
    assert(registry.select(DisplayDriverSDL3) == &sdl3);
    assert(registry.find(DisplayDriverX11) == &x11);
}

int main() {
    testComponentsDestroyCoordinatorBeforeBackendAndDevice();
    testRegistrySelectsAutoByRegistrationOrderAndExplicitDriver();
    return 0;
}
