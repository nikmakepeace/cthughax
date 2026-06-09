/** @file
 * Deterministic scene-script loading and playback.
 */

#ifndef CTHUGHA_SCENE_SCRIPT_H
#define CTHUGHA_SCENE_SCRIPT_H

#include "Configuration.h"

#include <cstddef>
#include <string>
#include <vector>

class SceneScriptEvent {
public:
    int step;
    int elapsedMs;
    int stop;
    std::string fileName;
    SceneConfig scene;

    SceneScriptEvent();
};

class SceneScriptLoadResult {
public:
    std::vector<SceneScriptEvent> events;
    std::vector<ConfigDiagnostic> diagnostics;

    bool ok() const;
};

class SceneScriptPlayback {
    std::vector<SceneScriptEvent> eventsValue;
    std::size_t nextIndexValue;
    int startedValue;
    double startSecondsValue;

public:
    SceneScriptPlayback();
    explicit SceneScriptPlayback(const std::vector<SceneScriptEvent>& events);

    int empty() const;
    int started() const;
    void reset();
    int elapsedMilliseconds(double nowSeconds);
    const SceneScriptEvent* nextDue(double nowSeconds);
    void consumeDue();
};

SceneScriptLoadResult loadSceneScript(const SceneScriptConfig& config);

#endif
