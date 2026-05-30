#include "cthugha.h"
#include "keymap.h"
#include "Interface.h"
#include "keys.h"
#include "AudioFrame.h"
#include "display.h"
#include "disp-sys.h"
#include "options.h"
#include "AutoChanger.h"
#include "AudioProcessor.h"
#include "AudioAnalyzer.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "Flashlight.h"
#include "VisualDirector.h"
#include "flames.h"
#include "waves.h"
#include "imath.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

void skipSpace(const char*& str) {
    while (isspace(*str))
        str++;
}

static int keymapFeatureValue(const char* name, int& known) {
    known = 1;

#ifdef WITH_CDROM
    if (strcasecmp(name, "WITH_CDROM") == 0)
        return WITH_CDROM;
#endif
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
#ifdef USE_XPM
    if (strcasecmp(name, "USE_XPM") == 0)
        return USE_XPM;
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

char Keymap::keymapFile[PATH_MAX] = "";

Action* Action::head = NULL;

Keymap* Keymap::first = NULL;
Keymap* Keymap::current = NULL;

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

//
// intializiation of the default keymaps
//
void Keymap::init() {

    static Keymap defaultKM("default");
    static Keymap mainKM("main");
    static Keymap helpKM("Help");
#if WITH_CDROM == 1
    static Keymap CDKM("CD");
#endif
    static Keymap soundKM("sound");
    static Keymap serverKM("server");
    static Keymap playListKM("playList");
    static Keymap optionsKM("Options");
    static Keymap coreOptionsKM("CoreOptions");
    static Keymap listKM("list");

    Keymap::addList(0,
#include <default.keymap.str>
        NULL);

    if (keymapFile[0] != '\0')
        readFile(keymapFile);
}

//
// all the actions implemented
//

ACTION(quit) { cthugha_close++; }

ACTION(screenChg) { screen.change(int(v), 0); }
ACTION(zoomChg) { zoom.change(int(v)); }
ACTION(flameChg) { flame.change(int(v), 0); }
ACTION(flameGeneral) { flameGeneral.changeRandom(); }
ACTION(waveChg) { wave.change(int(v), 0); }
ACTION(waveScaleChg) { waveScale.change(int(v), 0); }
ACTION(objectChg) { object.change(int(v), 0); }
ACTION(translateChg) { CthughaBuffer::current->translate.change(int(v), 0); }
ACTION(soundProcessChg) { audioProcessing.change(int(v)); }
ACTION(borderChg) { border.change(int(v), 0); }
ACTION(flashlightChg) { flashlight.change(int(v), 0); }
ACTION(paletteChg) { palette.change(int(v), 0); }
ACTION(deletePaletteChg) {
    PaletteEntry* paletteEntry = palette.currentPaletteEntry();
    if ((paletteEntry != NULL) && (paletteEntry->sourcePath[0] != '\0'))
        unlink(paletteEntry->sourcePath);

    palette.change(int(v), 0);
}
ACTION(tableChg) { table.change(int(v), 0); }
ACTION(pcxChg) { visualDirector().imageOption().change(int(v), 0); }
ACTION(lockChg) { lock.change(+1); }

ACTION(screen) { screen.change(p, 0); }
ACTION(zoom) { zoom.change(p); }
ACTION(flame) { flame.change(p, 0); }
ACTION(wave) { wave.change(p, 0); }
ACTION(waveScale) { waveScale.change(p, 0); }
ACTION(object) { object.change(p, 0); }
ACTION(translate) { CthughaBuffer::current->translate.change(p, 0); }
ACTION(soundProcess) { audioProcessing.change(p); }
ACTION(border) { border.change(p, 0); }
ACTION(flashlight) { flashlight.change(p, 0); }
ACTION(palette) { palette.change(p, 0); }
ACTION(table) { table.change(p, 0); }
ACTION(pcx) { visualDirector().imageOption().change(p, 0); }
ACTION(lock) { lock.change(+1); }

ACTION(writeIni) { write_ini(); } // when net-sound: re-sent request to server
ACTION(soundReset) { audioFrameChange(); }
ACTION(printScreen) { displayDevice->printScreen(); }

ACTION(restore) { CoreOption::restore(); }
ACTION(toggleSave) { Interface::saveToHot = 1 - Interface::saveToHot; }
ACTION(save) { CoreOption::save(int(v)); }
ACTION(saveOrRestore) {
    if (Interface::saveToHot) {
        CoreOption::save(int(v));
        Interface::saveToHot = 0;
    } else {
        CoreOption::restore(int(v));
    }
}

ACTION(toggleStatus) { Interface::showStatus = 1 - Interface::showStatus; }

ACTION(changeAll) { CoreOption::changeAll(); }
ACTION(changeOne) { CoreOption::changeOne(); }

ACTION(credits) { Interface::set("credits"); }
ACTION(setInterface) { Interface::set(p); }

ACTION(randomPalette) {
    PaletteEntry::Random();
    palette.setValue(PaletteEntry::lastRandomPos);
}
ACTION(newRandomPalette) {
    PaletteEntry::addRandom();
    palette.setValue(PaletteEntry::lastRandomPos);
}
