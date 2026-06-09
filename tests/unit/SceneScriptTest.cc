#include "SceneScript.h"

#include <assert.h>
#include <stdio.h>

#ifndef CTH_SOURCE_DIR
#define CTH_SOURCE_DIR "."
#endif

static std::string fixtureDir() {
    return std::string(CTH_SOURCE_DIR) + "/tests/fixtures/ini/perf";
}

static void loadsPerfSceneScript() {
    SceneScriptConfig config;
    config.directory = fixtureDir();
    config.script = "perf.scenescript";

    SceneScriptLoadResult result = loadSceneScript(config);

    assert(result.ok());
    assert(result.events.size() == 5);

    assert(result.events[0].step == 0);
    assert(result.events[0].elapsedMs == 0);
    assert(!result.events[0].stop);
    assert(result.events[0].fileName == "00.ini");
    assert(result.events[0].scene.flame == "0");
    assert(result.events[0].scene.translation == "0");
    assert(result.events[0].scene.presentation == "0");
    assert(result.events[0].scene.audioProcessing == "0");

    assert(result.events[3].step == 2);
    assert(result.events[3].elapsedMs == 6000);
    assert(result.events[3].fileName == "03.ini");
    assert(result.events[3].scene.flame == "3");
    assert(result.events[3].scene.translation == "2");
    assert(result.events[3].scene.presentation == "3");
    assert(result.events[3].scene.audioProcessing == "3");

    assert(result.events[4].step == 3);
    assert(result.events[4].elapsedMs == 8000);
    assert(result.events[4].stop);
}

static void playbackReturnsDueEventsInOrder() {
    std::vector<SceneScriptEvent> events;
    SceneScriptEvent first;
    first.step = 0;
    first.elapsedMs = 0;
    first.fileName = "00.ini";
    events.push_back(first);

    SceneScriptEvent second;
    second.step = 1;
    second.elapsedMs = 2000;
    second.fileName = "01.ini";
    events.push_back(second);

    SceneScriptPlayback playback(events);

    const SceneScriptEvent* due = playback.nextDue(10.0);
    assert(due != 0);
    assert(due->step == 0);
    playback.consumeDue();

    due = playback.nextDue(10.5);
    assert(due == 0);

    due = playback.nextDue(12.0);
    assert(due != 0);
    assert(due->step == 1);
    playback.consumeDue();

    assert(playback.nextDue(14.0) == 0);
}

int main() {
    fprintf(stderr, "%s", "");
    fflush(stderr);
    loadsPerfSceneScript();
    playbackReturnsDueEventsInOrder();
    return 0;
}
