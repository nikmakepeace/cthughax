/** @file
 * Deterministic scene-script loading and playback.
 */

#include "SceneScript.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

static std::string trim(const std::string& value) {
    std::string::size_type first = 0;
    while (first < value.size()
        && std::isspace(static_cast<unsigned char>(value[first])))
        first++;

    std::string::size_type last = value.size();
    while (last > first
        && std::isspace(static_cast<unsigned char>(value[last - 1])))
        last--;

    return value.substr(first, last - first);
}

static std::string removeInlineComment(const std::string& text) {
    std::string::size_type hash = text.find('#');
    std::string::size_type bang = text.find('!');
    std::string::size_type comment = std::string::npos;

    if (hash != std::string::npos)
        comment = hash;
    if (bang != std::string::npos)
        comment = std::min(comment, bang);

    return comment == std::string::npos ? text : text.substr(0, comment);
}

static bool parseInteger(const std::string& text, int* value) {
    std::istringstream input(text);
    int parsed = 0;
    char extra = 0;

    input >> parsed;
    if (!input || (input >> extra))
        return false;

    *value = parsed;
    return true;
}

static std::string joinedPath(const std::string& directory,
    const std::string& fileName) {
    if (directory.empty())
        return fileName;
    if (directory[directory.size() - 1] == '/')
        return directory + fileName;
    return directory + "/" + fileName;
}

static void appendDiagnostics(std::vector<ConfigDiagnostic>& destination,
    const std::vector<ConfigDiagnostic>& source) {
    destination.insert(destination.end(), source.begin(), source.end());
}

static std::vector<std::string> splitTokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream input(line);
    std::string token;

    while (input >> token)
        tokens.push_back(token);

    return tokens;
}

static bool assignSceneScriptToken(SceneScriptEvent& event,
    const std::string& source, int lineNumber, const std::string& key,
    const std::string& value, int& sawStep, int& sawElapsed, int& sawFile,
    DeferredLogBuffer& diagnostics) {
    int parsed = 0;

    if (key == "step") {
        if (!parseInteger(value, &parsed) || parsed < 0) {
            diagnostics.error(source, "scene-script",
                "line " + std::to_string(lineNumber)
                    + ": expected non-negative step");
            return false;
        }
        event.step = parsed;
        sawStep = 1;
        return true;
    }

    if (key == "elapsed-ms") {
        if (!parseInteger(value, &parsed) || parsed < 0) {
            diagnostics.error(source, "scene-script",
                "line " + std::to_string(lineNumber)
                    + ": expected non-negative elapsed-ms");
            return false;
        }
        event.elapsedMs = parsed;
        sawElapsed = 1;
        return true;
    }

    if (key == "file") {
        if (value.empty()) {
            diagnostics.error(source, "scene-script",
                "line " + std::to_string(lineNumber)
                    + ": expected file name");
            return false;
        }
        event.fileName = value;
        sawFile = 1;
        return true;
    }

    diagnostics.error(source, "scene-script",
        "line " + std::to_string(lineNumber) + ": unknown token `" + key
            + "'");
    return false;
}

static bool parseSceneScriptLine(const std::string& source, int lineNumber,
    const std::string& line, SceneScriptEvent& event,
    DeferredLogBuffer& diagnostics) {
    std::vector<std::string> tokens = splitTokens(line);
    int sawStep = 0;
    int sawElapsed = 0;
    int sawFile = 0;

    for (std::size_t i = 0; i < tokens.size(); i++) {
        std::string token = tokens[i];

        if (token == "stop") {
            event.stop = 1;
            continue;
        }

        std::string key;
        std::string value;
        std::string::size_type equals = token.find('=');
        if (equals != std::string::npos) {
            key = token.substr(0, equals);
            value = token.substr(equals + 1);
        } else {
            key = token;
            if (i + 1 >= tokens.size()) {
                diagnostics.error(source, "scene-script",
                    "line " + std::to_string(lineNumber)
                        + ": missing value for `" + key + "'");
                return false;
            }
            value = tokens[++i];
        }

        if (!assignSceneScriptToken(event, source, lineNumber, key, value,
                sawStep, sawElapsed, sawFile, diagnostics))
            return false;
    }

    if (!sawStep || !sawElapsed) {
        diagnostics.error(source, "scene-script",
            "line " + std::to_string(lineNumber)
                + ": expected step and elapsed-ms");
        return false;
    }
    if (event.stop == sawFile) {
        diagnostics.error(source, "scene-script",
            "line " + std::to_string(lineNumber)
                + ": expected exactly one of file or stop");
        return false;
    }

    return true;
}

static ConfigBuildResult loadSceneScriptIniFile(const std::string& path) {
    ConfigurationBuilder builder;
    return builder.addDefaults().addIniFile(path, false).build();
}

static void loadSceneScriptEventScene(const SceneScriptConfig& config,
    SceneScriptEvent& event, SceneScriptLoadResult& result) {
    if (event.stop)
        return;

    std::string path = joinedPath(config.directory, event.fileName);
    ConfigBuildResult sceneConfig = loadSceneScriptIniFile(path);
    appendDiagnostics(result.diagnostics, sceneConfig.diagnostics);
    event.scene = sceneConfig.config.scene;
}

} // namespace

SceneScriptEvent::SceneScriptEvent()
    : step(0)
    , elapsedMs(0)
    , stop(0)
    , fileName()
    , scene() { }

bool SceneScriptLoadResult::ok() const {
    for (std::vector<ConfigDiagnostic>::const_iterator it = diagnostics.begin();
         it != diagnostics.end(); ++it) {
        if (it->severity == ConfigDiagnosticError)
            return false;
    }
    return true;
}

SceneScriptPlayback::SceneScriptPlayback()
    : eventsValue()
    , nextIndexValue(0)
    , startedValue(0)
    , startSecondsValue(0.0) { }

SceneScriptPlayback::SceneScriptPlayback(
    const std::vector<SceneScriptEvent>& events)
    : eventsValue(events)
    , nextIndexValue(0)
    , startedValue(0)
    , startSecondsValue(0.0) { }

int SceneScriptPlayback::empty() const {
    return eventsValue.empty();
}

int SceneScriptPlayback::started() const {
    return startedValue;
}

void SceneScriptPlayback::reset() {
    nextIndexValue = 0;
    startedValue = 0;
    startSecondsValue = 0.0;
}

int SceneScriptPlayback::elapsedMilliseconds(double nowSeconds) {
    if (!startedValue) {
        startedValue = 1;
        startSecondsValue = nowSeconds;
    }

    double elapsedSeconds = nowSeconds - startSecondsValue;
    if (elapsedSeconds < 0.0)
        elapsedSeconds = 0.0;
    return int(elapsedSeconds * 1000.0);
}

const SceneScriptEvent* SceneScriptPlayback::nextDue(double nowSeconds) {
    if (nextIndexValue >= eventsValue.size())
        return 0;

    int elapsedMs = elapsedMilliseconds(nowSeconds);
    return eventsValue[nextIndexValue].elapsedMs <= elapsedMs
        ? &eventsValue[nextIndexValue]
        : 0;
}

void SceneScriptPlayback::consumeDue() {
    if (nextIndexValue < eventsValue.size())
        nextIndexValue++;
}

SceneScriptLoadResult loadSceneScript(const SceneScriptConfig& config) {
    SceneScriptLoadResult result;
    DeferredLogBuffer diagnostics;

    if (config.script.empty())
        return result;

    std::string scriptPath = joinedPath(config.directory, config.script);
    std::ifstream input(scriptPath.c_str(), std::ios::in | std::ios::binary);
    if (!input) {
        diagnostics.error(scriptPath, "scene-script",
            "could not open scene script");
        result.diagnostics = diagnostics.diagnostics();
        return result;
    }

    std::string line;
    int lineNumber = 0;
    int previousElapsedMs = -1;
    while (std::getline(input, line)) {
        lineNumber++;
        std::string cleaned = trim(removeInlineComment(line));
        if (cleaned.empty())
            continue;

        SceneScriptEvent event;
        if (!parseSceneScriptLine(scriptPath, lineNumber, cleaned, event,
                diagnostics))
            continue;

        if (event.elapsedMs < previousElapsedMs) {
            diagnostics.error(scriptPath, "scene-script",
                "line " + std::to_string(lineNumber)
                    + ": elapsed-ms must be nondecreasing");
            continue;
        }
        previousElapsedMs = event.elapsedMs;

        loadSceneScriptEventScene(config, event, result);
        result.events.push_back(event);
    }

    appendDiagnostics(result.diagnostics, diagnostics.diagnostics());
    return result;
}
