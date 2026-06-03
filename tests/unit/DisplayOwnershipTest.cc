#include "DisplayRuntime.h"

#include <assert.h>
#include <memory>
#include <vector>

static std::vector<int> destroyedObjects;

DisplayDevice* displayDevice = 0;
int DisplayDevice::text_on_term = 0;
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

    virtual DisplayEventStats processEvents() {
        return DisplayEventStats();
    }

    virtual PixelSize outputSize() const {
        return PixelSize(1, 1);
    }

    virtual void present(const DisplayPresentation&) {
    }
};

static void clearDisplayAliases() {
    displayDevice = 0;
    displayBackend = 0;
    displayRuntime = 0;
}

static void testPublishesAndClearsCompatibilityAliases() {
    clearDisplayAliases();

    std::unique_ptr<RecordingDevice> device(new RecordingDevice(1));
    RecordingDevice* deviceAlias = device.get();
    std::unique_ptr<RecordingBackend> backend(new RecordingBackend(2));
    RecordingBackend* backendAlias = backend.get();
    std::unique_ptr<DisplayRuntime> runtime(new DisplayRuntime(*backend));
    DisplayRuntime* runtimeAlias = runtime.get();

    DisplayRuntimeOwnership ownership(std::move(device), std::move(backend),
        std::move(runtime));

    assert(&ownership.device() == deviceAlias);
    assert(&ownership.backend() == backendAlias);
    assert(&ownership.runtime() == runtimeAlias);

    ownership.publishAliases();

    assert(displayDevice == deviceAlias);
    assert(displayBackend == backendAlias);
    assert(displayRuntime == runtimeAlias);

    ownership.shutdown();

    assert(displayDevice == 0);
    assert(displayBackend == 0);
    assert(displayRuntime == 0);
}

static void testShutdownDestroysBackendBeforeDeviceAndIsIdempotent() {
    clearDisplayAliases();
    destroyedObjects.clear();

    std::unique_ptr<RecordingDevice> device(new RecordingDevice(1));
    std::unique_ptr<RecordingBackend> backend(new RecordingBackend(2));
    std::unique_ptr<DisplayRuntime> runtime(new DisplayRuntime(*backend));

    DisplayRuntimeOwnership ownership(std::move(device), std::move(backend),
        std::move(runtime));
    ownership.publishAliases();

    ownership.shutdown();
    ownership.shutdown();

    assert(destroyedObjects.size() == 2);
    assert(destroyedObjects[0] == 2);
    assert(destroyedObjects[1] == 1);
}

int main() {
    testPublishesAndClearsCompatibilityAliases();
    testShutdownDestroysBackendBeforeDeviceAndIsIdempotent();
    return 0;
}
