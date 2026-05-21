#include "cthugha.h"
#include "keymap.h"
#include "Interface.h"
#include "keys.h"
#include "Sound.h"
#include "display.h"
#include "disp-sys.h"
#include "options.h"
#include "AutoChanger.h"
#include "SoundProcess.h"
#include "SoundAnalyze.h"
#include "CthughaBuffer.h"
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

    while (!feof(file)) {
        const char* lineP = line;

        fgets(line, 4096, file);
        char* N = strchr(line, '\n'); // delete the trailing newline
        if (N)
            *N = '\0';
        lineNr++;
        if (ferror(file)) {
            CTH_ERRNO(errno, "Error while reading keymap file '%s' at line %d.", fileName, lineNr);
            fclose(file);
            return;
        }

        if (strncasecmp("#keymap", line, 7) == 0) {
            lineP = line + 7;
            skipSpace(lineP);
            keymap = find(lineP, 1);

            CTH_DEBUG("      defining keymap '%s'.\n", keymap->name);
        } else {
            keymap->add(line);
        }
    }

    CTH_DEBUG("   finished reading keymap file '%s'.\n", fileName);
    fclose(file);
}

void Keymap::addList(int dummy, ...) {

    va_list ap;
    va_start(ap, dummy); /* Initialize the argument list. */

    Keymap* keymap = find("default");

    const char* line = va_arg(ap, const char*);
    while (line != NULL) {
        const char* lineP;

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
    CTH_TRACE("   parsed new keymap entry line '%s'\n", fullLine);
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
    static Keymap CDKM("CD");
    static Keymap soundKM("sound");
    static Keymap serverKM("server");
    static Keymap playListKM("playList");
    static Keymap optionsKM("Options");
    static Keymap coreOptionsKM("CoreOptions");
    static Keymap listKM("list");

    Keymap::addList(0,
#include "default.keymap.str"
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
ACTION(bufferChg) { CthughaBuffer::nCurrent.change(int(v)); }
ACTION(flameChg) { CthughaBuffer::current->flame.change(int(v), 0); }
ACTION(flameGeneral) { CthughaBuffer::current->flameGeneral.changeRandom(); }
ACTION(waveChg) { CthughaBuffer::current->wave.change(int(v), 0); }
ACTION(waveScaleChg) { CthughaBuffer::current->waveScale.change(int(v), 0); }
ACTION(objectChg) { CthughaBuffer::current->object.change(int(v), 0); }
ACTION(translateChg) { CthughaBuffer::current->translate.change(int(v), 0); }
ACTION(soundProcessChg) { CthughaBuffer::current->soundProcess.change(int(v), 0); }
ACTION(borderChg) { CthughaBuffer::current->border.change(int(v), 0); }
ACTION(flashlightChg) { CthughaBuffer::current->flashlight.change(int(v), 0); }
ACTION(paletteChg) { CthughaBuffer::current->palette.change(int(v), 0); }
ACTION(deletePaletteChg) {
    PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette.current();
    if ((palette != NULL) && (palette->sourcePath[0] != '\0'))
        unlink(palette->sourcePath);

    CthughaBuffer::current->palette.change(int(v), 0);
}
ACTION(tableChg) { CthughaBuffer::current->table.change(int(v), 0); }
ACTION(pcxChg) { CthughaBuffer::current->pcx.change(int(v), 0); }
ACTION(lockChg) { lock.change(+1); }
ACTION(lightChg) { light.change(int(v), 0); }
ACTION(backgroundChg) { background.change(int(v), 0); }
ACTION(flyChg) { fly.change(int(v), 0); }

ACTION(screen) { screen.change(p, 0); }
ACTION(zoom) { zoom.change(p); }
ACTION(buffer) { CthughaBuffer::nCurrent.change(int(v)); }
ACTION(flame) { CthughaBuffer::current->flame.change(p, 0); }
ACTION(wave) { CthughaBuffer::current->wave.change(p, 0); }
ACTION(waveScale) { CthughaBuffer::current->waveScale.change(p, 0); }
ACTION(object) { CthughaBuffer::current->object.change(p, 0); }
ACTION(translate) { CthughaBuffer::current->translate.change(p, 0); }
ACTION(soundProcess) { CthughaBuffer::current->soundProcess.change(p, 0); }
ACTION(border) { CthughaBuffer::current->border.change(p, 0); }
ACTION(flashlight) { CthughaBuffer::current->flashlight.change(p, 0); }
ACTION(palette) { CthughaBuffer::current->palette.change(p, 0); }
ACTION(table) { CthughaBuffer::current->table.change(p, 0); }
ACTION(pcx) { CthughaBuffer::current->pcx.change(p, 0); }
ACTION(lock) { lock.change(+1); }
ACTION(light) { light.change(p, 0); }
ACTION(background) { background.change(p, 0); }
ACTION(fly) { fly.change(p, 0); }

ACTION(writeIni) { write_ini(); } // when net-sound: re-sent request to server
ACTION(soundReset) { soundDevice->change(); }
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
    CthughaBuffer::current->palette.setValue(PaletteEntry::lastRandomPos);
}
ACTION(newRandomPalette) {
    PaletteEntry::addRandom();
    CthughaBuffer::current->palette.setValue(PaletteEntry::lastRandomPos);
}
