/** @file
 * Integration coverage for the visualiser control service and panel client.
 */

#include "ControlPanelClient.h"
#include "ControlDisplayCatalogs.h"
#include "ControlService.h"
#include "ControlRuntimeMetrics.h"
#include "ControlTransport.h"
#include "ProcessServices.h"
#include "RuntimeConfigRegistry.h"
#include "RuntimeCommandSink.h"
#include "SceneChoiceSelection.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

class QuietLogSink : public LogSink {
public:
    virtual int enabled(int) const {
        return 0;
    }

protected:
    virtual void write(int, const char*, int, const char*, va_list) {
    }
};

class RecordingLauncher : public ControlPanelProcessLauncher {
public:
    int calls;
    int terminateCalls;
    std::string endpointValue;

    RecordingLauncher()
        : calls(0)
        , terminateCalls(0)
        , endpointValue() { }

    virtual bool launchPanel(
        const std::string& endpoint, std::string*) {
        calls++;
        endpointValue = endpoint;
        return true;
    }

    virtual void terminatePanel() {
        terminateCalls++;
    }
};

class FakeChoice : public SceneChoice {
    std::string nameValue;
    int inUseValue;

public:
    FakeChoice(const char* name_, int inUse_)
        : nameValue(name_)
        , inUseValue(inUse_) { }

    virtual const char* name() const { return nameValue.c_str(); }
    virtual int sameName(const char* other) const {
        return nameValue == (other != 0 ? other : "");
    }
    virtual int inUse() const { return inUseValue; }
    virtual void setUse(int inUse) { inUseValue = inUse; }
};

class FakeSelection : public SceneFlameSelection,
                      public SceneGeneralFlameSelection,
                      public SceneWaveSelection,
                      public SceneWaveObjectSelection,
                      public SceneTranslationSelection,
                      public ScenePaletteSelection,
                      public SceneImageSelection {
    std::vector<FakeChoice> choices;
    int currentValueValue;

public:
    FakeSelection()
        : choices()
        , currentValueValue(0) {
        choices.push_back(FakeChoice("first", 1));
        choices.push_back(FakeChoice("second", 1));
    }

    virtual const char* currentName() const {
        return choices[currentValueValue].name();
    }
    virtual int currentValue() const { return currentValueValue; }
    virtual int entryCount() const { return int(choices.size()); }
    virtual int choiceCount() const { return int(choices.size()); }
    virtual SceneChoice* choiceAt(int index) {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return &choices[index];
    }
    virtual const SceneChoice* choiceAt(int index) const {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return &choices[index];
    }
    virtual void change(int by) {
        currentValueValue = (currentValueValue + by) % int(choices.size());
    }
    virtual void change(const char*, RandomSource&) { currentValueValue = 1; }
    virtual int changeRandom(RandomSource&) { currentValueValue = 0; return 1; }
    virtual void setValue(int index) { currentValueValue = index; }
    virtual const Flame* currentFlame() { return 0; }
    virtual int encodedValue() const { return currentValueValue; }
    virtual const char* selectionText() const { return currentName(); }
    virtual Wave* currentWave() { return 0; }
    virtual WObject* currentObject() { return 0; }
    virtual TranslationTable currentTranslationTable() {
        return TranslationTable();
    }
    virtual PaletteEntry* currentPaletteEntry() { return 0; }
    virtual const IndexedImage* currentImage() { return 0; }
};

class FakeSelections : public SceneVisualSelections {
    FakeSelection selection;

public:
    virtual SceneFlameSelection& flame() { return selection; }
    virtual SceneGeneralFlameSelection& generalFlame() { return selection; }
    virtual SceneWaveSelection& wave() { return selection; }
    virtual SceneOptionSelection& waveScale() { return selection; }
    virtual SceneOptionSelection& table() { return selection; }
    virtual SceneOptionSelection& object() { return selection; }
    virtual SceneTranslationSelection& translation() { return selection; }
    virtual ScenePaletteSelection& palette() { return selection; }
    virtual SceneOptionSelection& border() { return selection; }
    virtual SceneOptionSelection& flashlight() { return selection; }
    virtual SceneImageSelection& images() { return selection; }
};

class FakeDisplayCatalogs : public ControlDisplayCatalogs {
public:
    virtual int screenChoiceCount() const { return 2; }
    virtual const char* screenChoiceNameAt(int index) const {
        return index == 0 ? "Up" : "Source";
    }
    virtual const char* screenChoiceLabelAt(int index) const {
        return index == 0 ? "Up Display" : "Source size";
    }
    virtual int screenChoiceInUseAt(int) const { return 1; }
};

class FakeRuntimeMetrics : public ControlRuntimeMetrics {
public:
    ControlRuntimeMetricsSnapshot snapshotValue;

    FakeRuntimeMetrics()
        : snapshotValue(0, 0, 100) { }

    virtual ControlRuntimeMetricsSnapshot snapshot() const {
        return snapshotValue;
    }
};

static Config sampleConfig() {
    Config config;
    config.scene.flame = "first";
    config.scene.translation = "first";
    config.scene.image = "first";
    config.scene.object = "first";
    config.scene.table = "first";
    config.scene.waveScale = "first";
    config.scene.palette = "first";
    config.scene.flashlight = "off";
    config.scene.presentation = "Up";
    config.scene.audioProcessing = "none";
    config.display.maxFramesPerSecond = 25;
    config.audioAnalysis.fireSensitivity = 100;
    config.audioAnalysis.fireSource = "raw-amplitude";
    config.autoChange.cumulativeFireLevel = 1000;
    return config;
}

class UpdatingRuntimeSink : public RuntimeCommandSink {
    RuntimeConfigRegistry& registry;
    Config configValue;

public:
    int calls;
    RuntimeCommandType lastType;
    std::string lastText;
    int lastValue;

    UpdatingRuntimeSink(RuntimeConfigRegistry& registry_, const Config& config_)
        : registry(registry_)
        , configValue(config_)
        , calls(0)
        , lastType(RuntimeCommandChangeAll)
        , lastText()
        , lastValue(0) { }

    virtual RuntimeChangeSet apply(const RuntimeCommand& command) {
        RuntimeChangeSet changes;
        calls++;
        lastType = command.type;
        lastText = command.text != 0 ? command.text : "";
        lastValue = command.value;

        if (command.type == RuntimeCommandChangeMaxFpsTo) {
            configValue.display.maxFramesPerSecond = command.value;
            registry.setBaseline(configValue);
            changes.displayChanged = 1;
            changes.fpsChanged = 1;
        } else if (command.type == RuntimeCommandChangeSceneTo) {
            if (command.sceneTarget == RuntimeSceneFlame)
                configValue.scene.flame = lastText;
            registry.setBaseline(configValue);
            changes.sceneChanges = 1;
        } else if (command.type == RuntimeCommandChangeSoundProcessingTo) {
            configValue.scene.audioProcessing = lastText;
            registry.setBaseline(configValue);
            changes.audioProcessingChanged = 1;
        } else if (command.type == RuntimeCommandChangeFireSensitivityTo) {
            configValue.audioAnalysis.fireSensitivity = command.value;
            registry.setBaseline(configValue);
            changes.audioProcessingChanged = 1;
        } else if (command.type == RuntimeCommandChangeFireSourceTo) {
            configValue.audioAnalysis.fireSource = lastText;
            registry.setBaseline(configValue);
            changes.audioProcessingChanged = 1;
        } else if (command.type == RuntimeCommandChangeScreenTo) {
            configValue.scene.presentation = lastText;
            registry.setBaseline(configValue);
            changes.displayChanged = 1;
        } else if (command.type == RuntimeCommandChangeAutoChangeLockTo) {
            configValue.autoChange.locked = command.value;
            registry.setBaseline(configValue);
            changes.autoChangeChanged = 1;
        } else if (command.type
            == RuntimeCommandChangeAutoChangeCumulativeFireLevelTo) {
            configValue.autoChange.cumulativeFireLevel = command.value;
            registry.setBaseline(configValue);
            changes.autoChangeChanged = 1;
        }

        return changes;
    }

    void setAppFlame(const char* flame) {
        configValue.scene.flame = flame;
        registry.setBaseline(configValue);
    }
};

struct ObservedMessages {
    int connected;
    int disconnected;
    int errors;
    std::vector<ControlJsonValue> messages;

    ObservedMessages()
        : connected(0)
        , disconnected(0)
        , errors(0)
        , messages() { }
};

static std::string typeOf(const ControlJsonValue& message) {
    const ControlJsonValue* type = message.member("type");
    if (type == 0 || type->type() != ControlJsonValue::StringType)
        return "";
    return type->asString();
}

static const ControlJsonValue* latestMessageOfType(
    const ObservedMessages& observed, const char* type) {
    for (std::vector<ControlJsonValue>::const_reverse_iterator it
             = observed.messages.rbegin();
         it != observed.messages.rend(); ++it) {
        if (typeOf(*it) == type)
            return &*it;
    }
    return 0;
}

static int latestStateMaxFps(
    const ObservedMessages& observed, int fallback) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* display
        = state != 0 ? state->member("display") : 0;
    const ControlJsonValue* maxFps
        = display != 0 ? display->member("maxFps") : 0;
    return maxFps != 0 ? int(maxFps->asNumber(fallback)) : fallback;
}

static std::string latestStateFlame(const ObservedMessages& observed) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* scene = state != 0 ? state->member("scene") : 0;
    const ControlJsonValue* flame = scene != 0 ? scene->member("flame") : 0;
    return flame != 0 ? flame->asString() : "";
}

static std::string latestStateScreen(const ObservedMessages& observed) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* display
        = state != 0 ? state->member("display") : 0;
    const ControlJsonValue* screen
        = display != 0 ? display->member("screen") : 0;
    return screen != 0 ? screen->asString() : "";
}

static int latestStateAutoChangeEnabled(
    const ObservedMessages& observed, int fallback) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* autoChange
        = state != 0 ? state->member("autoChange") : 0;
    const ControlJsonValue* enabled
        = autoChange != 0 ? autoChange->member("enabled") : 0;
    return enabled != 0 ? (enabled->asBool() ? 1 : 0) : fallback;
}

static int latestStateAutoChangeCumulativeFire(
    const ObservedMessages& observed, int fallback) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* autoChange
        = state != 0 ? state->member("autoChange") : 0;
    const ControlJsonValue* threshold
        = autoChange != 0 ? autoChange->member("cumulativeFireLevel") : 0;
    return threshold != 0 ? int(threshold->asNumber(fallback)) : fallback;
}

static int latestStateAudioFireSensitivity(
    const ObservedMessages& observed, int fallback) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* audio = state != 0 ? state->member("audio") : 0;
    const ControlJsonValue* sensitivity
        = audio != 0 ? audio->member("fireSensitivity") : 0;
    return sensitivity != 0 ? int(sensitivity->asNumber(fallback)) : fallback;
}

static std::string latestStateAudioFireSource(
    const ObservedMessages& observed) {
    const ControlJsonValue* state = latestMessageOfType(observed, "state");
    const ControlJsonValue* audio = state != 0 ? state->member("audio") : 0;
    const ControlJsonValue* source
        = audio != 0 ? audio->member("fireSource") : 0;
    return source != 0 ? source->asString() : "";
}

static int latestAckId(const ObservedMessages& observed) {
    const ControlJsonValue* ack = latestMessageOfType(observed, "ack");
    const ControlJsonValue* id = ack != 0 ? ack->member("id") : 0;
    return id != 0 ? int(id->asNumber(0)) : 0;
}

static void pump(ControlService& service, ControlPanelClient& client,
    ObservedMessages& observed, int milliseconds) {
    std::chrono::steady_clock::time_point deadline
        = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() < deadline) {
        service.serviceFrame(8);
        std::vector<ControlPanelClientEvent> events = client.pollEvents();
        for (std::vector<ControlPanelClientEvent>::const_iterator it
                 = events.begin();
             it != events.end(); ++it) {
            if (it->type == ControlPanelClientEvent::Connected)
                observed.connected++;
            else if (it->type == ControlPanelClientEvent::Disconnected)
                observed.disconnected++;
            else if (it->type == ControlPanelClientEvent::Error)
                observed.errors++;
            else if (it->type == ControlPanelClientEvent::Message)
                observed.messages.push_back(it->message);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

static std::string uniqueInstanceId() {
    std::ostringstream out;
    out << "integration-"
        << std::chrono::steady_clock::now().time_since_epoch().count();
    return out.str();
}

static void assertServiceStarted(ControlService& service, const std::string& id) {
    std::string error;
    if (!service.start(id, &error)) {
        fprintf(stderr, "control service start failed: %s\n",
            error.c_str());
        assert(0);
    }
}

static void testServiceClientSynchronizesBothDirections() {
#ifndef _WIN32
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);
#endif
    Config config = sampleConfig();
    RuntimeConfigRegistry registry(config);
    UpdatingRuntimeSink runtimeSink(registry, config);
    FakeSelections selections;
    FakeDisplayCatalogs displayCatalogs;
    FakeRuntimeMetrics runtimeMetrics;
    QuietLogSink log;
    RecordingLauncher launcher;
    ControlService service(runtimeSink, registry, selections, displayCatalogs,
        runtimeMetrics, log, launcher);

    assertServiceStarted(service, uniqueInstanceId());

    service.launchControlPanel();
    service.launchControlPanel();
    assert(launcher.calls == 1);
    assert(launcher.endpointValue == service.endpoint());

    ControlPanelClient client(service.endpoint());
    client.start();
    ObservedMessages observed;
    pump(service, client, observed, 500);

    assert(observed.connected == 1);
    assert(observed.errors == 0);
    assert(latestMessageOfType(observed, "catalogs") != 0);
    assert(latestStateMaxFps(observed, -1) == 25);

    service.launchControlPanel();
    assert(launcher.calls == 1);

    int id = client.sendSetNumber("display.maxFps", 72);
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 1);
    assert(runtimeSink.lastType == RuntimeCommandChangeMaxFpsTo);
    assert(runtimeSink.lastValue == 72);
    assert(latestAckId(observed) == id);
    assert(latestStateMaxFps(observed, -1) == 72);

    id = client.sendSet("display.screen", "Source");
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 2);
    assert(runtimeSink.lastType == RuntimeCommandChangeScreenTo);
    assert(runtimeSink.lastText == "Source");
    assert(latestAckId(observed) == id);
    assert(latestStateScreen(observed) == "Source");

    id = client.sendSetBool("autoChange.enabled", false);
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 3);
    assert(runtimeSink.lastType == RuntimeCommandChangeAutoChangeLockTo);
    assert(runtimeSink.lastValue == 1);
    assert(latestAckId(observed) == id);
    assert(latestStateAutoChangeEnabled(observed, -1) == 0);

    id = client.sendSetNumber("autoChange.cumulativeFireLevel", 420);
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 4);
    assert(runtimeSink.lastType
        == RuntimeCommandChangeAutoChangeCumulativeFireLevelTo);
    assert(runtimeSink.lastValue == 420);
    assert(latestAckId(observed) == id);
    assert(latestStateAutoChangeCumulativeFire(observed, -1) == 420);

    id = client.sendSetNumber("audio.fireSensitivity", 37);
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 5);
    assert(runtimeSink.lastType == RuntimeCommandChangeFireSensitivityTo);
    assert(runtimeSink.lastValue == 37);
    assert(latestAckId(observed) == id);
    assert(latestStateAudioFireSensitivity(observed, -1) == 37);

    id = client.sendSet("audio.fireSource", "low-pass-150hz-amplitude");
    pump(service, client, observed, 500);
    assert(runtimeSink.calls == 6);
    assert(runtimeSink.lastType == RuntimeCommandChangeFireSourceTo);
    assert(runtimeSink.lastText == "low-pass-150hz-amplitude");
    assert(latestAckId(observed) == id);
    assert(latestStateAudioFireSource(observed)
        == "low-pass-150hz-amplitude");

    runtimeSink.setAppFlame("second");
    service.runtimeStateChanged();
    pump(service, client, observed, 500);
    assert(latestStateFlame(observed) == "second");

    client.stop();
    service.stop();
    assert(launcher.terminateCalls == 1);
}

static void testServiceFrameIsBoundedWithStalledClient() {
#ifndef _WIN32
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);
#endif
    Config config = sampleConfig();
    RuntimeConfigRegistry registry(config);
    UpdatingRuntimeSink runtimeSink(registry, config);
    FakeSelections selections;
    FakeDisplayCatalogs displayCatalogs;
    FakeRuntimeMetrics runtimeMetrics;
    QuietLogSink log;
    RecordingLauncher launcher;
    ControlService service(runtimeSink, registry, selections, displayCatalogs,
        runtimeMetrics, log, launcher);

    std::string error;
    assertServiceStarted(service, uniqueInstanceId());
    std::unique_ptr<ControlStream> stalled = connectControlEndpoint(
        service.endpoint(), 1000, &error);
    assert(stalled.get() != 0);

    std::chrono::steady_clock::time_point started
        = std::chrono::steady_clock::now();
    for (int i = 0; i < 200; i++) {
        service.runtimeStateChanged();
        service.serviceFrame(4);
    }
    std::chrono::steady_clock::duration elapsed
        = std::chrono::steady_clock::now() - started;

    stalled->close();
    service.stop();

    assert(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)
            .count()
        < 250);
}

int main() {
    testServiceClientSynchronizesBothDirections();
    testServiceFrameIsBoundedWithStalledClient();
    return 0;
}
