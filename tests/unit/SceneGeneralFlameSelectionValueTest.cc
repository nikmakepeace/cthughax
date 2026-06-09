#include "SceneGeneralFlameSelectionValue.h"

#include "ProcessServices.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include <string>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

class RecordingLock : public SceneChoiceLock {
public:
    int enabledValue;

    explicit RecordingLock(int enabled_ = 0)
        : enabledValue(enabled_) { }

    virtual int enabled() const { return enabledValue; }

    virtual void change(const char* to) {
        enabledValue = std::strcmp(to, "on") == 0;
    }
};

class FixedRandomSource : public RandomSource {
    int value;

public:
    explicit FixedRandomSource(int value_)
        : value(value_) { }

    virtual int uniformInt(int) { return value; }
};

class RecordingLogSink : public LogSink {
public:
    int warnCalls;
    std::string lastMessage;

    RecordingLogSink()
        : warnCalls(0)
        , lastMessage() { }

    virtual int enabled(int) const { return 1; }

protected:
    virtual void write(int level, const char*, int, const char* fmt,
        va_list ap) {
        if (level == CTH_LOG_WARN)
            warnCalls++;
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, ap);
        lastMessage = buffer;
    }
};

static void testSelectionWrapsEncodedValue() {
    SceneGeneralFlameSelectionValue selection(
        "flame-general", new RecordingLock(), 0);

    assert(std::strcmp(selection.catalogName(), "flame-general") == 0);
    assert(selection.entryCount() == 59049);

    selection.setValue(59050);
    assert(selection.encodedValue() == 1);
    assert(selection.currentValue() == 1);
    assert(std::strcmp(selection.selectionText(), "1") == 0);

    selection.change(-2);
    assert(selection.encodedValue() == 59048);
}

static void testSelectionParsesLockPrefixes() {
    SceneGeneralFlameSelectionValue selection(
        "flame-general", new RecordingLock(), 4);
    FixedRandomSource randomSource(123);

    selection.change("locked:42", randomSource);
    assert(selection.encodedValue() == 42);
    assert(selection.lockEnabled() == 1);
    assert(std::strcmp(selection.currentName(), "locked:42") == 0);

    selection.change("no-lock:17", randomSource);
    assert(selection.encodedValue() == 17);
    assert(selection.lockEnabled() == 0);
    assert(std::strcmp(selection.currentName(), "17") == 0);

    selection.toggleLock();
    assert(selection.lockEnabled() == 1);
    assert(std::strcmp(selection.selectionText(), "locked:17") == 0);
}

static void testInvalidTextUsesInjectedRandomSourceAndDiagnostics() {
    RecordingLogSink log;
    SceneGeneralFlameSelectionValue selection(
        "flame-general", new RecordingLock(), 4, &log);
    FixedRandomSource randomSource(123);

    selection.change("not-a-number", randomSource);

    assert(selection.encodedValue() == 123);
    assert(log.warnCalls == 1);
    assert(log.lastMessage.find("not-a-number") != std::string::npos);
}

static void testRandomChangeRespectsLock() {
    SceneGeneralFlameSelectionValue selection(
        "flame-general", new RecordingLock(), 4);
    FixedRandomSource randomSource(123);

    assert(selection.changeRandom(randomSource) == 1);
    assert(selection.encodedValue() == 123);

    selection.toggleLock();
    assert(selection.changeRandom(randomSource) == 0);
    assert(selection.encodedValue() == 123);
}

static void testActivationUsesEncodedRange() {
    SceneGeneralFlameSelectionValue selection(
        "flame-general", new RecordingLock(), 4);

    selection.activate(59048);
    assert(selection.encodedValue() == 59048);

    selection.activate(59049);
    assert(selection.encodedValue() == 59048);
}

int main() {
    testSelectionWrapsEncodedValue();
    testSelectionParsesLockPrefixes();
    testInvalidTextUsesInjectedRandomSourceAndDiagnostics();
    testRandomChangeRespectsLock();
    testActivationUsesEncodedRange();
    return 0;
}
