#include "cthugha.h"
#include "Configuration.h"
#include "keymap.h"
#include "Interface.h"
#include "keys.h"
#include "display.h"
#include "disp-sys.h"
#include "AudioProcessing.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "Flashlight.h"
#include "VideoDirector.h"
#include "flames.h"
#include "waves.h"
#include "imath.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "Scene.h"
#include "RuntimeCommandSink.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <unistd.h>

void skipSpace(const char*& str) {
    while (isspace(*str))
        str++;
}

static int keymapFeatureValue(const char* name, int& known) {
    known = 1;

#ifdef WITH_DSP
    if (strcasecmp(name, "WITH_DSP") == 0)
        return WITH_DSP;
#endif
#ifdef WITH_MIXER
    if (strcasecmp(name, "WITH_MIXER") == 0)
        return WITH_MIXER;
#endif
#ifdef WITH_PULSE
    if (strcasecmp(name, "WITH_PULSE") == 0)
        return WITH_PULSE;
#endif
#ifdef WITH_MINIMP3
    if (strcasecmp(name, "WITH_MINIMP3") == 0)
        return WITH_MINIMP3;
#endif
    known = 0;
    return 0;
}

static long keymapConditionTerm(const char*& str, int& known) {
    skipSpace(str);

    char* end = NULL;
    long value = strtol(str, &end, 0);
    if (end != str) {
        str = end;
        known = 1;
        return value;
    }

    char name[128];
    int i = 0;
    while ((isalnum(*str) || (*str == '_')) && (i < int(sizeof(name)) - 1)) {
        name[i++] = *str;
        str++;
    }
    name[i] = '\0';

    if (i == 0) {
        known = 0;
        return 0;
    }

    int featureKnown = 0;
    value = keymapFeatureValue(name, featureKnown);
    if (!featureKnown)
        CTH_WARN("Unknown keymap condition feature '%s'.\n", name);

    known = featureKnown;
    return value;
}

static int keymapConditionValue(const char* str) {
    int negate = 0;
    skipSpace(str);
    if (*str == '!') {
        negate = 1;
        str++;
    }

    int known = 0;
    long left = keymapConditionTerm(str, known);
    int result = known && (left != 0);

    skipSpace(str);
    if ((strncmp(str, "==", 2) == 0) || (strncmp(str, "!=", 2) == 0)) {
        int equals = (str[0] == '=');
        str += 2;

        int rightKnown = 0;
        long right = keymapConditionTerm(str, rightKnown);
        result = known && rightKnown && (equals ? (left == right) : (left != right));
    } else if (*str != '\0') {
        CTH_WARN("Unsupported keymap condition expression near '%s'.\n", str);
        result = 0;
    }

    return negate ? !result : result;
}

static int keymapDirective(const char*& str, const char* directive) {
    int len = strlen(directive);
    if (strncasecmp(str, directive, len) != 0)
        return 0;

    if ((str[len] != '\0') && !isspace(str[len]))
        return 0;

    str += len;
    skipSpace(str);
    return 1;
}

class KeymapConditionState {
    struct Frame {
        int parentActive;
        int conditionActive;
        int seenElse;
    };

    enum { MaxDepth = 32 };

    Frame stack[MaxDepth];
    int depth;
    int currentActive;

public:
    KeymapConditionState()
        : depth(0)
        , currentActive(1) { }

    int active() const { return currentActive; }

    int handle(const char* rawLine) {
        const char* line = rawLine;
        skipSpace(line);

        if (*line != '#')
            return 0;

        line++;

        if (keymapDirective(line, "if")) {
            if (depth >= MaxDepth) {
                CTH_ERROR("Too many nested keymap conditionals.\n");
                currentActive = 0;
                return 1;
            }

            int conditionActive = keymapConditionValue(line);
            stack[depth].parentActive = currentActive;
            stack[depth].conditionActive = conditionActive;
            stack[depth].seenElse = 0;
            currentActive = currentActive && conditionActive;
            depth++;
            return 1;
        }

        if (keymapDirective(line, "else")) {
            if (depth == 0) {
                CTH_ERROR("Unexpected #else in keymap.\n");
                return 1;
            }

            Frame& frame = stack[depth - 1];
            if (frame.seenElse)
                CTH_ERROR("Duplicate #else in keymap conditional.\n");
            frame.seenElse = 1;
            currentActive = frame.parentActive && !frame.conditionActive;
            return 1;
        }

        if (keymapDirective(line, "endif")) {
            if (depth == 0) {
                CTH_ERROR("Unexpected #endif in keymap.\n");
                return 1;
            }

            depth--;
            currentActive = stack[depth].parentActive;
            return 1;
        }

        return 0;
    }

    void finish() const {
        if (depth != 0)
            CTH_ERROR("Unterminated #if in keymap.\n");
    }
};

Action* Action::head = NULL;

Keymap* Keymap::first = NULL;
Keymap* Keymap::current = NULL;
static RuntimeCommandSink* keymapRuntimeCommandSink = NULL;
static const AudioProcessingState* keymapAudioProcessingState = NULL;

Keymap::Keymap(const char* n)
    : next(first)
    , name(n)
    , bindingList(NULL) {
    first = this;
}

void Keymap::readFile(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        CTH_ERRNO(errno, "Can not open keymap file '%s'.", fileName);
        return;
    }

    CTH_DEBUG("   starting reading keymap file '%s'.\n", fileName);

    int lineNr = 0;
    char line[4096];
    Keymap* keymap = find("default");
    KeymapConditionState conditions;

    while (fgets(line, 4096, file) != NULL) {
        const char* lineP = line;

        char* N = strchr(line, '\n'); // delete the trailing newline
        if (N)
            *N = '\0';
        lineNr++;

        if (conditions.handle(line))
            continue;

        if (!conditions.active())
            continue;

        if (strncasecmp("#keymap", line, 7) == 0) {
            lineP = line + 7;
            skipSpace(lineP);
            keymap = find(lineP, 1);

            CTH_DEBUG("      defining keymap '%s'.\n", keymap->name);
        } else {
            keymap->add(line);
        }
    }

    if (ferror(file)) {
        CTH_ERRNO(errno, "Error while reading keymap file '%s' at line %d.", fileName, lineNr);
        fclose(file);
        return;
    }

    conditions.finish();

    CTH_DEBUG("   finished reading keymap file '%s'.\n", fileName);
    fclose(file);
}

void Keymap::addList(int dummy, ...) {

    va_list ap;
    va_start(ap, dummy); /* Initialize the argument list. */

    Keymap* keymap = find("default");
    KeymapConditionState conditions;

    const char* line = va_arg(ap, const char*);
    while (line != NULL) {
        const char* lineP;

        if (conditions.handle(line)) {
            line = va_arg(ap, const char*);
            continue;
        }

        if (!conditions.active()) {
            line = va_arg(ap, const char*);
            continue;
        }

        if (strncasecmp("#keymap", line, 7) == 0) {
            lineP = line + 7;
            skipSpace(lineP);
            keymap = find(lineP);

            CTH_DEBUG("      defining keymap '%s'.\n", keymap->name);
        } else {
            keymap->add(line);
        }

        line = va_arg(ap, const char*);
    }

    conditions.finish();

    va_end(ap);
}

Keymap* Keymap::find(const char* name, int create) {
    for (Keymap* k = first; k; k = k->next) {
        if (strcasecmp(name, k->name) == 0) {
            return k;
        }
    }

    if (create) {
        CTH_DEBUG("      adding new keymap '%s'.\n", name);
        char* n = new char[strlen(name) + 1];
        strcpy(n, name);
        return new Keymap(n);
    } else {
        CTH_ERROR("Could not find keymap '%s'.\n", name);
        return first;
    }
}

void Keymap::add(Binding& b) {
    // try to replace an existing key
    BindingList* l = bindingList;
    while (l) {
        if (l->key == b.key) {
            l->actionList = b.actionList;
            //	    delete l->param;
            //	    l->param = b.param;
            return;
        }
        l = l->next;
    }
    // add a new key
    bindingList = new BindingList(b, bindingList);
}

Keymap::Binding Keymap::parseBinding(const char* line) {
    const char* fullLine = line;
    Binding b;

    skipSpace(line);

    switch (*line) {
    case '\0':
    case '#':
    case '!':
        return b;
    case '\\':
        if (line[1] == '\0')
            return b;

        b.key = line[1];
        line++;
        break;

    default:
        b.key = *line;
    }

    line++;
    if ((*line != '\0') && !isspace(*line)) { // a multi-character thing
        line--;

        for (int i = 0; i < nKeyAssoc; i++)
            if (strncasecmp(keyAssoc[i].name, line, strlen(keyAssoc[i].name)) == 0) {
                b.key = keyAssoc[i].keyValue;
                line += strlen(keyAssoc[i].name);
                i = nKeyAssoc + 1;
            }

        if ((*line != '\0') && !isspace(*line)) {
            CTH_ERROR("Unknown key symbol in keymap line: '%s'\n", fullLine);
            b.key = 0;
            return b;
        }
    }
    skipSpace(line);

    b.actionList = NULL; // no actions so far
    while ((*line) != '\0') {

        // try to match an action
        Action* A = NULL;
        int l = -1; // find longest match
        for (Action* a = Action::head; a != NULL; a = a->next) {
            if (((*a) == line) && (a->nameLen > l)) {
                A = a;
                l = a->nameLen;
            }
        }
        if (A) { // found some match
            line += A->nameLen; // skip command name

            skipSpace(line);

            if (*line != '(') { // check and skip '('
                CTH_ERROR("Missing '(' in binding: '%s'\n", fullLine);
                b.key = 0;
                return b;
            }
            line++;

            if (strchr(line, ')') == NULL) { // check and skip ')'
                CTH_ERROR("Missing ')' in binding: '%s'\n", fullLine);
                b.key = 0;
                return b;
            }

            int n = strchr(line, ')') - line; // parse parameter
            char* p = new char[n + 1];
            strncpy(p, line, n);
            p[n] = '\0';

            line += n + 1; // skip parameter and ')'

            // add new action to List
            b.actionList = new Binding::ActionList(A, p, b.actionList);
        } else {
            CTH_ERROR("Error in keymap. Unknown command: '%s'\n", line);
            b.key = 0;
            return b;
        }
        skipSpace(line);
    }
    CTH_DEBUG("keymap: parsed new keymap entry line '%s'\n", fullLine);
    return b;
}

int Keymap::action(int key) {

    BindingList* l = bindingList;
    while (l) {
        if (l->key == key) {
            for (Binding::ActionList* a = l->actionList; a != NULL; a = a->next) {
                double value;
                if (a->param)
                    sscanf(a->param, "%lf", &value);

                a->action->act(a->param, value);
            }
            return 0;
        }
        l = l->next;
    }
    return 1;
}

int Keymap::action(const char* keymap, int key) {

    for (Keymap* k = first; k; k = k->next) {
        if (strcasecmp(keymap, k->name) == 0) {
            return k->action(key);
        }
    }
    return 1;
}

void Keymap::add(const char* line) {
    Binding b = parseBinding(line);
    if (b.key != 0)
        add(b);
}

void Keymap::set(const char* name) {

    Keymap* k = first;
    while (k) {
        if (strcasecmp(name, k->name) == 0) {
            current = k;
            return;
        }
        k = k->next;
    }
}

void Keymap::setRuntimeCommandSink(RuntimeCommandSink* sink) {
    keymapRuntimeCommandSink = sink;
}

RuntimeCommandSink* Keymap::runtimeCommandSink() {
    return keymapRuntimeCommandSink;
}

void Keymap::setAudioProcessingState(const AudioProcessingState* state) {
    keymapAudioProcessingState = state;
}

const AudioProcessingState* Keymap::audioProcessingState() {
    return keymapAudioProcessingState;
}

//
// intializiation of the default keymaps
//
void Keymap::init(const InputConfig& config) {

    static Keymap defaultKM("default");
    static Keymap mainKM("main");
    static Keymap helpKM("Help");
    static Keymap soundKM("sound");
    static Keymap serverKM("server");
    static Keymap playListKM("playList");
    static Keymap optionsKM("Options");
    static Keymap effectControlionsKM("EffectControls");
    static Keymap listKM("list");

    Keymap::addList(0,
#include <default.keymap.str>
        NULL);

    if (!config.keymapFile.empty())
        readFile(config.keymapFile.c_str());
}

//
// all the actions implemented
//

static std::string continuation_name(const char* name) {
    if (name == NULL || strcmp(name, "unknown") == 0)
        return "";

    return name;
}

static RuntimeContinuationState current_continuation_state() {
    RuntimeContinuationState state;
    SceneCommands* sceneCommands = sceneCommandsForLegacyCallbacks();
    if (sceneCommands != NULL) {
        const SceneSettings& settings = sceneCommands->sceneState().settings();
        state.flame = continuation_name(settings.flameName);
        state.generalFlame = continuation_name(settings.generalFlameName);
        state.wave = continuation_name(settings.waveName);
        state.waveScale = continuation_name(settings.waveScaleName);
        state.object = continuation_name(settings.objectName);
        state.translation = continuation_name(settings.translationName);
        state.palette = continuation_name(settings.paletteName);
        state.border = continuation_name(settings.borderName);
        state.flashlight = continuation_name(settings.flashlightName);
        state.table = continuation_name(settings.tableName);
        state.image = continuation_name(sceneCommands->imageOption().currentName());
    }

    state.presentation = continuation_name(screen.currentName());
    const AudioProcessingState* audioState = Keymap::audioProcessingState();
    state.audioProcessing = continuation_name(
        audioState != NULL ? audioState->text() : "");
    state.showFpsEnabled = int(showFPS);
    return state;
}

static void applyRuntimeCommand(const RuntimeCommand& command) {
    RuntimeCommandSink* sink = Keymap::runtimeCommandSink();
    if (sink != NULL)
        sink->apply(command);
}

ACTION(quit) { applyRuntimeCommand(RuntimeCommand::requestClose()); }
ACTION(stopAndContinue) {
    applyRuntimeCommand(RuntimeCommand::stopAndContinue(
        current_continuation_state()));
}

ACTION(screenChg) { applyRuntimeCommand(RuntimeCommand::changeScreenBy(int(v))); }
ACTION(zoomChg) { applyRuntimeCommand(RuntimeCommand::changeZoomBy(int(v))); }
ACTION(flameChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneFlame, int(v))); }
ACTION(flameGeneral) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneGeneralFlame, 0)); }
ACTION(waveChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneWave, int(v))); }
ACTION(waveScaleChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneWaveScale, int(v))); }
ACTION(objectChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneObject, int(v))); }
ACTION(translateChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneTranslation, int(v))); }
ACTION(soundProcessChg) { applyRuntimeCommand(RuntimeCommand::changeSoundProcessingBy(int(v))); }
ACTION(borderChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneBorder, int(v))); }
ACTION(flashlightChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneFlashlight, int(v))); }
ACTION(paletteChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeScenePalette, int(v))); }
ACTION(tableChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneTable, int(v))); }
ACTION(imageChg) { applyRuntimeCommand(RuntimeCommand::changeSceneBy(RuntimeSceneImage, int(v))); }
ACTION(lockChg) { applyRuntimeCommand(RuntimeCommand::toggleAutoChangeLock()); }

ACTION(screen) { applyRuntimeCommand(RuntimeCommand::changeScreenTo(p)); }
ACTION(zoom) { applyRuntimeCommand(RuntimeCommand::changeZoomTo(p)); }
ACTION(flame) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneFlame, p)); }
ACTION(wave) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneWave, p)); }
ACTION(waveScale) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneWaveScale, p)); }
ACTION(object) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneObject, p)); }
ACTION(translate) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneTranslation, p)); }
ACTION(soundProcess) { applyRuntimeCommand(RuntimeCommand::changeSoundProcessingTo(p)); }
ACTION(border) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneBorder, p)); }
ACTION(flashlight) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneFlashlight, p)); }
ACTION(palette) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeScenePalette, p)); }
ACTION(table) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneTable, p)); }
ACTION(image) { applyRuntimeCommand(RuntimeCommand::changeSceneTo(RuntimeSceneImage, p)); }
ACTION(lock) { applyRuntimeCommand(RuntimeCommand::toggleAutoChangeLock()); }

ACTION(writeIni) { applyRuntimeCommand(RuntimeCommand::writeIni()); }

ACTION(restore) { applyRuntimeCommand(RuntimeCommand::restoreScene()); }
ACTION(toggleSave) { Interface::saveToPreset = 1 - Interface::saveToPreset; }
ACTION(save) { applyRuntimeCommand(RuntimeCommand::savePreset(int(v))); }
ACTION(saveOrRestore) {
    if (Interface::saveToPreset) {
        applyRuntimeCommand(RuntimeCommand::savePreset(int(v)));
        Interface::saveToPreset = 0;
    } else {
        applyRuntimeCommand(RuntimeCommand::restorePreset(int(v)));
    }
}

ACTION(toggleStatus) { Interface::showStatus = 1 - Interface::showStatus; }
ACTION(toggleFPS) { applyRuntimeCommand(RuntimeCommand::toggleShowFps()); }

ACTION(changeAll) { applyRuntimeCommand(RuntimeCommand::changeAll()); }
ACTION(changeOne) { applyRuntimeCommand(RuntimeCommand::changeOne()); }

ACTION(credits) { Interface::set("credits"); }
ACTION(setInterface) { Interface::set(p); }

ACTION(randomPalette) {
    applyRuntimeCommand(RuntimeCommand::randomPalette());
}
ACTION(newRandomPalette) {
    applyRuntimeCommand(RuntimeCommand::addRandomPalette());
}
