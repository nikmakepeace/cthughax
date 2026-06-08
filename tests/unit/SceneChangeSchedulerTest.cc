#include "AutoChangeSettings.h"
#include "AudioAnalyzer.h"
#include "ProcessServices.h"
#include "Scene.h"
#include "SceneChangeScheduler.h"

#include <assert.h>
#include <string.h>
#include <vector>

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

class NullLogSink : public LogSink {
public:
    virtual int enabled(int) const { return 0; }

protected:
    virtual void write(int, const char*, int, const char*, va_list) { }
};

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) { }

AcousticContext::AcousticContext(LogSink* log_)
    : log(log_)
    , intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) { }

void AcousticContext::update(const AudioMetrics&) { }
double AcousticContext::intensity() const { return 0.0; }
int AcousticContext::fire() const { return 0; }
int AcousticContext::cumulativeFireLevel() const { return 0; }
void AcousticContext::resetCumulativeFireLevel() { }

class RecordingSceneCommands : public SceneCommandTarget {
public:
    int changeOneCalls;
    int changeAllCalls;

    RecordingSceneCommands()
        : changeOneCalls(0)
        , changeAllCalls(0) { }

    virtual void changeOne() { changeOneCalls++; }
    virtual void changeAll() { changeAllCalls++; }
    virtual void restore() { }
    virtual void restorePreset(int) { }
    virtual void savePreset(int) { }
    virtual void randomPalette() { }
    virtual void addRandomPalette() { }
    virtual void change(SceneSelectionTarget, int) { }
    virtual void change(SceneSelectionTarget, const char*) { }
    virtual void activate(SceneSelectionTarget, int) { }
    virtual void toggleLock(SceneSelectionTarget) { }
    virtual void toggleChoiceUse(SceneSelectionTarget, int) { }
};

class RecordingQuietObserver : public AutoChangeQuietObserver {
public:
    int calls;
    int lastQuietLength;
    int consumeQuietPeriod;

    RecordingQuietObserver()
        : calls(0)
        , lastQuietLength(0)
        , consumeQuietPeriod(0) { }

    virtual int observeQuiet(int quietLength) {
        calls++;
        lastQuietLength = quietLength;
        return consumeQuietPeriod;
    }
};

class FakeClock : public MillisecondClock {
public:
    int value;

    FakeClock()
        : value(1000) { }

    virtual int milliseconds() const {
        return value;
    }
};

class FakeRandomSource : public RandomSource {
public:
    int value;
    int calls;

    FakeRandomSource()
        : value(0)
        , calls(0) { }

    virtual int uniformInt(int exclusiveMax) {
        calls++;
        if (exclusiveMax <= 1)
            return 0;
        return value % exclusiveMax;
    }
};

static AutoChangeConfig autoChangeConfigWithLittle(int changeLittle) {
    AutoChangeConfig config;
    config.quietMs = 0;
    config.waitMinMs = 0;
    config.waitRandomMs = 1;
    config.cumulativeFireLevel = 0;
    config.locked = 0;
    config.changeLittle = changeLittle;
    return config;
}

static void testSceneChangeSchedulerRequestsChangeOneForLittleChanges() {
    OwnedAutoChangeSettings settings(autoChangeConfigWithLittle(1));

    RecordingSceneCommands sceneCommands;
    AcousticContext acousticContext;
    FakeClock clock;
    FakeRandomSource randomSource;
    NullLogSink log;
    RecordingQuietObserver quietObserver;
    SceneChangeScheduler scheduler(sceneCommands, settings, acousticContext,
        clock, randomSource, quietObserver, log);
    scheduler.change();

    assert(sceneCommands.changeOneCalls == 1);
    assert(sceneCommands.changeAllCalls == 0);
}

static void testSceneChangeSchedulerRequestsChangeAllForFullChanges() {
    OwnedAutoChangeSettings settings(autoChangeConfigWithLittle(0));

    RecordingSceneCommands sceneCommands;
    AcousticContext acousticContext;
    FakeClock clock;
    FakeRandomSource randomSource;
    NullLogSink log;
    RecordingQuietObserver quietObserver;
    SceneChangeScheduler scheduler(sceneCommands, settings, acousticContext,
        clock, randomSource, quietObserver, log);
    scheduler.change();

    assert(sceneCommands.changeOneCalls == 0);
    assert(sceneCommands.changeAllCalls == 1);
}

static AutoChangeConfig autoChangeConfigWithWait() {
    AutoChangeConfig config = autoChangeConfigWithLittle(1);
    config.waitMinMs = 10;
    config.waitRandomMs = 5;
    return config;
}

static void testSceneChangeSchedulerUsesInjectedClockAndRandomForWaitChanges() {
    OwnedAutoChangeSettings settings(autoChangeConfigWithWait());
    RecordingSceneCommands sceneCommands;
    AcousticContext acousticContext;
    FakeClock clock;
    FakeRandomSource randomSource;
    AudioMetrics metrics;
    NullLogSink log;
    RecordingQuietObserver quietObserver;

    randomSource.value = 3;
    SceneChangeScheduler scheduler(sceneCommands, settings, acousticContext,
        clock, randomSource, quietObserver, log);
    assert(randomSource.calls == 1);

    clock.value = 1013;
    scheduler(metrics);
    assert(sceneCommands.changeOneCalls == 0);
    assert(sceneCommands.changeAllCalls == 0);

    clock.value = 1014;
    scheduler(metrics);
    assert(sceneCommands.changeOneCalls == 1);
    assert(sceneCommands.changeAllCalls == 0);
    assert(randomSource.calls == 2);
}

static void testSceneChangeSchedulerUsesInjectedQuietObserver() {
    AutoChangeConfig config = autoChangeConfigWithLittle(1);
    config.locked = 1;
    OwnedAutoChangeSettings settings(config);
    RecordingSceneCommands sceneCommands;
    AcousticContext acousticContext;
    FakeClock clock;
    FakeRandomSource randomSource;
    RecordingQuietObserver quietObserver;
    AudioMetrics metrics;
    NullLogSink log;

    SceneChangeScheduler scheduler(sceneCommands, settings, acousticContext,
        clock, randomSource, quietObserver, log);
    clock.value = 1250;
    quietObserver.consumeQuietPeriod = 1;

    scheduler(metrics);

    assert(quietObserver.calls == 1);
    assert(quietObserver.lastQuietLength == 250);
    assert(sceneCommands.changeOneCalls == 0);
    assert(sceneCommands.changeAllCalls == 0);
}

static void testSceneChangeSchedulerStatusTextIsInstanceLocal() {
    AutoChangeConfig config = autoChangeConfigWithLittle(1);
    config.locked = 1;
    OwnedAutoChangeSettings settings(config);
    RecordingSceneCommands firstSceneCommands;
    RecordingSceneCommands secondSceneCommands;
    AcousticContext firstAcousticContext;
    AcousticContext secondAcousticContext;
    FakeClock firstClock;
    FakeClock secondClock;
    FakeRandomSource firstRandomSource;
    FakeRandomSource secondRandomSource;
    RecordingQuietObserver firstQuietObserver;
    RecordingQuietObserver secondQuietObserver;
    NullLogSink log;
    SceneChangeScheduler first(firstSceneCommands, settings, firstAcousticContext,
        firstClock, firstRandomSource, firstQuietObserver, log);
    SceneChangeScheduler second(secondSceneCommands, settings,
        secondAcousticContext, secondClock, secondRandomSource,
        secondQuietObserver, log);

    const char* firstStatus = first.status();
    const char* secondStatus = second.status();

    assert(firstStatus != secondStatus);
    assert(strcmp(firstStatus, "locked ") == 0);
    assert(strcmp(secondStatus, "locked ") == 0);
    assert(strcmp(first.sceneChangeStatus(), "locked ") == 0);
}

int main() {
    testSceneChangeSchedulerRequestsChangeOneForLittleChanges();
    testSceneChangeSchedulerRequestsChangeAllForFullChanges();
    testSceneChangeSchedulerUsesInjectedClockAndRandomForWaitChanges();
    testSceneChangeSchedulerUsesInjectedQuietObserver();
    testSceneChangeSchedulerStatusTextIsInstanceLocal();
    return 0;
}
