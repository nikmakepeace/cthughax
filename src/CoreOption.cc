#include "cthugha.h"
#include "CoreOption.h"
#include "imath.h"
#include "CthughaDisplay.h"
#include "options.h"
#include "display.h"
#include "CthughaBuffer.h"

#include <unistd.h>
#include <string>

static const int visualBufferCount = 1;

// autoconf suggests this
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

OptionOnOff double_load("double-load", DEFAULT_DOUBLE_LOAD_ENABLED); // allow double loading of features

CoreOption* CoreOption::first = NULL;

const int MAX_HISTORY = 128;
const int MAX_HOT = 10;

static int compareFeatureFileNames(const void* a, const void* b) {
    const char* left = *(const char* const*)a;
    const char* right = *(const char* const*)b;
    int folded = strcasecmp(left, right);

    return folded ? folded : strcmp(left, right);
}

CoreOption::CoreOption(int b, const char* n, CoreOptionEntryList& e, int flags_)
    : Option(n)
    , buffer(b)
    , entries(e)
    , flags(flags_)
    , lock("", 0) {

    // keep in linked list
    next = first;
    first = this;

    initialEntry[0] = '\0';

    // history
    oldValues = new int[MAX_HISTORY];
    history = 0;

    // hot values
    hot = new int[MAX_HOT];
    for (int i = 0; i < MAX_HOT; i++)
        hot[i] = 0;
}

CoreOption& CoreOption::operator=(const CoreOption& other) {
    entries = other.entries;

    return *this;
}

const char* CoreOption::name() const {
    static char str[512];

    if (buffer < 0)
        return _name;

    sprintf(str, "%s.%d", _name, buffer);
    return str;
}

//
// Changeing
//
void CoreOption::changeToInitial() {
    for (CoreOption* o = first; o != NULL; o = o->next) {
        o->change(o->initialEntry, 0);
    }
}

void CoreOption::change(int by, int doSave) {

    if (doSave)
        save();

    // check for empty set
    if (getNEntries() == 0)
        return;

    /* check, if there is any feature we can use */
    int nouse = 1;
    for (int i = 0; i < getNEntries(); i++)
        if (int(entries[i]->use)) {
            i = getNEntries();
            nouse = 0;
        }

    if (nouse) { // if none in use, use the first
        value = 0;
    } else {
        value = mod(value + by, getNEntries());
        if (by < 0) {
            while (!int(entries[value]->use))
                value = mod(value - 1, getNEntries());
        } else
            while (!int(entries[value]->use))
                value = mod(value + 1, getNEntries());
    }

    CTH_DEBUG("changed option `%s' to `%s'\n", name(), entries[value]->name);

    if (cthughaDisplay)
        cthughaDisplay->resetFPS();
}

void CoreOption::changeRandom(int doSave) {
    if (lock)
        return;
    value = mod(rand(), getNEntries()); // select a desired value
    change(0, doSave); // change to next usable value, do saving
}

void CoreOption::change(const char* to, int doSave) {
    char* pos;

    // check, if there is something we can set this to
    if (getNEntries() == 0) {
        return;
    }

    /* if empty, set to a random value */
    if ((to == NULL) || (to[0] == '\0')) {
        CTH_DEBUG("changing option `%s' to a random value.\n", name());
        value = Random(getNEntries());
        change(0, 0);
        return;
    }

    CTH_DEBUG("changing option `%s' to `%s'.\n", name(), to);

    if (doSave)
        save();

    // possible prefixes to tun on locking
    static const char* Lon[] = { "l:", "lock:", "locked:" };
    static int nLon = 3;
    // possible prefixes to turn off locking
    static const char* Loff[] = { "no-l:", "no-lock:", "no-locked:", "nol:", "nolock:", "nolocked:",
        "non-l:", "non-lock:", "non-locked:", "nonl:", "nonlock:", "nonlocked:" };
    static int nLoff = sizeof(Loff) / sizeof(const char*);

    // check if tun on prefix is given
    for (int i = 0; i < nLon; i++) {
        if (strncasecmp(to, Lon[i], strlen(Lon[i])) == 0) {
            lock.change("on");
            to = to + strlen(Lon[i]);
            i = nLon;
        }
    }
    // check if turn off prefix is given
    for (int i = 0; i < nLoff; i++) {
        if (strncasecmp(to, Loff[i], strlen(Loff[i])) == 0) {
            lock.change("off");
            to = to + strlen(Loff[i]);
            i = nLoff;
        }
    }

    // try to find it as a name
    for (value = 0; value < getNEntries(); value++)
        if (entries[value]->sameName(to)) { // found tha name
            entries[value]->use.setValue(1);
            return;
        }

    // not a name, try as number
    value = strtol(to, &pos, 0);

    if (pos == to) { // not a number
        /* found no entry, use a random value */
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to, name());
        value = mod(rand(), getNEntries());
    } else { // it is a number
        value = mod(value, getNEntries());
    }

    entries[value]->use.setValue(1);
}

//
// convert an option name to a number
//
int CoreOption::optNr(const char* n) {
    char* pos;

    // check, if there is something we can set this to
    if (getNEntries() == 0) {
        return 0;
    }

    /* if empty, set to a random value */
    if ((n == NULL) || (n[0] == '\0')) {
        return Random(getNEntries());
    }

    // try to find it as a name
    for (int v = 0; v < getNEntries(); v++)
        if (entries[v]->sameName(n)) { // found tha name
            return v;
        }

    // not a name, try as number
    int v = strtol(n, &pos, 0);

    if (pos == n) { // not a number
        return Random(getNEntries());
    } else {
        return mod(v, getNEntries());
    }
}

//
// change randomly one of the features
//
CoreOption* CoreOption::changeOne() {

    int nCandidates = 0;
    for (CoreOption* o = first; o != NULL; o = o->next) {
        if (o->isAutoChangeCandidate())
            nCandidates++;
    }

    if (nCandidates == 0)
        return 0;

    int n = rand() % nCandidates;
    CoreOption* o = CoreOption::first;
    while (o != NULL) {
        if (o->isAutoChangeCandidate()) {
            if (n == 0)
                break;
            n--;
        }
        o = o->next;
    }

    if (o == NULL) {
        CTH_ERROR("internal error: no CoreOption found among %d autochange candidates\n",
            nCandidates);
        return 0;
    }

    if (o->lock)
        return 0;
    o->changeRandom(1);
    return o;
}

//
// change all the features
//
void CoreOption::changeAll() {

    save();

    for (CoreOption* o = first; o != NULL; o = o->next) {
        if (o->isAutoChangeCandidate())
            o->changeRandom(0);
    }
}

int CoreOption::isAutoChangeCandidate() const {
    if (!autoChangeEnabled())
        return 0;

    return (buffer < 0) || (buffer < visualBufferCount);
}

const char* CoreOption::text() const {
    static std::string text;

    text.clear();
    if (lock)
        text = "locked:";

    text += currentName();

    if (currentDesc()[0] != '\0') {
        text += " (";
        text += currentDesc();
        text += ")";
    }

    return text.c_str();
}

//
// save the current state
//
void CoreOption::save() {
    for (CoreOption* o = first; o != NULL; o = o->next)
        o->doSave();
}
void CoreOption::doSave() {
    if (history >= MAX_HISTORY) {
        memcpy(oldValues, oldValues + 1, sizeof(int) * (MAX_HISTORY - 1));
        history--;
    }
    oldValues[history] = value;
    history++;
}

//
// get back the last state
//
void CoreOption::restore() {
    for (CoreOption* o = first; o != NULL; o = o->next)
        o->doRestore();
}
void CoreOption::doRestore() {
    if (history > 0) {
        history--;
        value = int(oldValues[history]);
        this->change(0, 0);
    }
}

//
// save to hotkey position
//
void CoreOption::save(int to) {
    if ((to < 0) || (to >= MAX_HOT))
        return;

    for (CoreOption* o = first; o != NULL; o = o->next)
        o->hot[to] = o->value;
}

//
// get back from hotkey
//
void CoreOption::restore(int from) {
    if ((from < 0) || (from >= MAX_HOT))
        return;

    save();

    for (CoreOption* o = first; o != NULL; o = o->next) {
        o->value = o->hot[from];
        o->change(0, 0);
    }
}

void CoreOption::setHotValue(int slot, int value_) {
    if ((slot < 0) || (slot >= MAX_HOT))
        return;

    hot[slot] = value_;
}

int CoreOption::hotValue(int slot) const {
    if ((slot < 0) || (slot >= MAX_HOT))
        return 0;

    return hot[slot];
}

int CoreOption::hotSlotCount() { return MAX_HOT; }

CoreOption* CoreOption::firstRegistered() { return first; }

//
// add a new entry to the list
//
void CoreOption::add(CoreOptionEntry* entry) { entries.add(entry); }
void CoreOption::add(CoreOptionEntry** entries, int nEntries) {
    for (int i = 0; i < nEntries; i++)
        add(entries[i]);
}

//
// check if an entry is already defines
//
int CoreOption::defined(const char* name) {

    for (int i = 0; i < getNEntries(); i++)
        if (strcmp(entries[i]->name, name) == 0)
            return 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * general load-function
 */

int CoreOption::isCompressed(const char* name) {
    int l = strlen(name);
    if (l < 3)
        return 0;
    if ((name[l - 3] == '.') && (name[l - 2] == 'g') && (name[l - 1] == 'z'))
        return 1;
    return 0;
}

int CoreOption::hasExtension(const char* name, const char* extension) {
    const char* pos = NULL;
    size_t name_len = strlen(name);
    size_t extension_len = strlen(extension);

    if (name_len < extension_len)
        return 0;

    for (size_t i = 0; i + extension_len <= name_len; i++) {
        if (strncasecmp(name + i, extension, extension_len) == 0) {
            pos = name + i;
            break;
        }
    }

    if (pos == NULL)
        return 0;

    /* check if extension is at end of name, or is followed by a . */
    if ((pos[extension_len] == '\0') || (pos[extension_len] == '.'))
        return 1;

    return 0;
}

CoreOptionEntry* CoreOption::load(const char* name, char* total_name, const char* dir,
    CoreOptionLoader loader) {

    int compressed = isCompressed(total_name);
    FILE* file;

    if (compressed) {
        /* open with 'gzip' - through pipe */
        char cmd[PATH_MAX];

        CTH_DEBUG("uncompressing and ");

        sprintf(cmd, "gzip -cd \"%s\"", total_name);

        file = popen(cmd, "r");

    } else {
        /* normal open - as file */
        file = fopen(total_name, "r");
    }

    CTH_DEBUG("loading: %s", total_name);

    /* now do the loading */
    if (file != NULL) {
        CoreOptionEntry* entry;

        /* file was openen successfully  - now read it */
        if ((entry = (*loader)(file, name, dir, total_name)) != NULL) {
            CTH_DEBUG(" ... OK\n");
        }

        if (compressed)
            pclose(file); /* close pipe */
        else
            fclose(file); /* close file */

        return entry;

    } else { /* file/pipe could not be opened */
        CTH_DEBUG(" ... x\n");
        return NULL;
    }
}

CoreOptionEntry* CoreOption::load(const char* name, char* total_name, const char* dir,
    CoreOptionContextLoader loader, void* context) {

    int compressed = isCompressed(total_name);
    FILE* file;

    if (compressed) {
        /* open with 'gzip' - through pipe */
        char cmd[PATH_MAX];

        CTH_DEBUG("uncompressing and ");

        sprintf(cmd, "gzip -cd \"%s\"", total_name);

        file = popen(cmd, "r");

    } else {
        /* normal open - as file */
        file = fopen(total_name, "r");
    }

    CTH_DEBUG("loading: %s", total_name);

    /* now do the loading */
    if (file != NULL) {
        CoreOptionEntry* entry;

        /* file was openen successfully  - now read it */
        if ((entry = (*loader)(file, name, dir, total_name, context)) != NULL) {
            CTH_DEBUG(" ... OK\n");
        }

        if (compressed)
            pclose(file); /* close pipe */
        else
            fclose(file); /* close file */

        return entry;

    } else { /* file/pipe could not be opened */
        CTH_DEBUG(" ... x\n");
        return NULL;
    }
}

void CoreOption::loadDir(const char* dir, const char* extension,
    CoreOptionLoader loader) {
    char total_name[PATH_MAX];
    char feat_name[PATH_MAX];
    DIR* directory;
    struct dirent* entry;
    CoreOptionEntry* fe;
    char** names = NULL;
    int nNames = 0;
    int maxNames = 0;

    if ((directory = opendir(dir)) != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (!hasExtension(entry->d_name, extension))
                continue;

            if (nNames >= maxNames) {
                int newMaxNames = maxNames ? maxNames * 2 : 32;
                char** newNames = (char**)realloc(names, newMaxNames * sizeof(char*));
                if (newNames == NULL) {
                    CTH_ERRNO(errno, "Can not allocate feature filename list.");
                    break;
                }
                names = newNames;
                maxNames = newMaxNames;
            }

            names[nNames] = new char[strlen(entry->d_name) + 1];
            strcpy(names[nNames], entry->d_name);
            nNames++;
        }
        closedir(directory);

        qsort(names, nNames, sizeof(char*), compareFeatureFileNames);

        for (int i = 0; i < nNames; i++) {
            /* create real filename */
            snprintf(total_name, PATH_MAX, "%s%s", dir, names[i]);

            CTH_DEBUG("    ");

            /* feature name only goes till first occurence of extension */
            strncpy(feat_name, names[i], PATH_MAX);
            feat_name[PATH_MAX - 1] = '\0';
            for (char* pos = feat_name; *pos != '\0'; pos++) {
                if (strncasecmp(pos, extension, strlen(extension)) == 0) {
                    *pos = '\0';
                    break;
                }
            }

            if (feat_name[0] == '\0') {
                CTH_DEBUG("skipping empty feature name: %s\n", total_name);
                delete[] names[i];
                continue;
            }

            /*
             * Plain and compressed files share the same feature name:
             * `foo.pcx' and `foo.pcx.gz' both become `foo'. This also
             * applies to other asset extensions such as `foo.png'. Without
             * --dbl-load, the alphabetically first filename wins and later
             * duplicates are skipped.
             */
            if (!int(double_load) && defined(feat_name)) {
                CTH_DEBUG("already loaded: %s\n", total_name);
            } else if ((fe = load(feat_name, total_name, dir, loader)) != NULL)
                add(fe);

            delete[] names[i];
        }
        free(names);
    }
}

void CoreOption::loadDir(const char* dir, const char* extension,
    CoreOptionContextLoader loader, void* context) {
    char total_name[PATH_MAX];
    char feat_name[PATH_MAX];
    DIR* directory;
    struct dirent* entry;
    CoreOptionEntry* fe;
    char** names = NULL;
    int nNames = 0;
    int maxNames = 0;

    if ((directory = opendir(dir)) != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (!hasExtension(entry->d_name, extension))
                continue;

            if (nNames >= maxNames) {
                int newMaxNames = maxNames ? maxNames * 2 : 32;
                char** newNames = (char**)realloc(names, newMaxNames * sizeof(char*));
                if (newNames == NULL) {
                    CTH_ERRNO(errno, "Can not allocate feature filename list.");
                    break;
                }
                names = newNames;
                maxNames = newMaxNames;
            }

            names[nNames] = new char[strlen(entry->d_name) + 1];
            strcpy(names[nNames], entry->d_name);
            nNames++;
        }
        closedir(directory);

        qsort(names, nNames, sizeof(char*), compareFeatureFileNames);

        for (int i = 0; i < nNames; i++) {
            /* create real filename */
            snprintf(total_name, PATH_MAX, "%s%s", dir, names[i]);

            CTH_DEBUG("    ");

            /* feature name only goes till first occurence of extension */
            strncpy(feat_name, names[i], PATH_MAX);
            feat_name[PATH_MAX - 1] = '\0';
            for (char* pos = feat_name; *pos != '\0'; pos++) {
                if (strncasecmp(pos, extension, strlen(extension)) == 0) {
                    *pos = '\0';
                    break;
                }
            }

            if (feat_name[0] == '\0') {
                CTH_DEBUG("skipping empty feature name: %s\n", total_name);
                delete[] names[i];
                continue;
            }

            /*
             * Plain and compressed files share the same feature name:
             * `foo.pcx' and `foo.pcx.gz' both become `foo'. This also
             * applies to other asset extensions such as `foo.png'. Without
             * --dbl-load, the alphabetically first filename wins and later
             * duplicates are skipped.
             */
            if (!int(double_load) && defined(feat_name)) {
                CTH_DEBUG("already loaded: %s\n", total_name);
            } else if ((fe = load(feat_name, total_name, dir, loader, context)) != NULL)
                add(fe);

            delete[] names[i];
        }
        free(names);
    }
}

int CoreOption::load(const char* searchPath[], const char* extraPath, const char* extension,
    CoreOptionLoader loader) {
    int path;

    /* load normal search path */
    path = 0;
    while (searchPath[path][0] != '\0') {
        loadDir(searchPath[path], extension, loader);
        path++;
    }
    /* load from extra path */
    if (extra_lib_path[0] != '\0') {
        std::string extraPathDir = std::string(extra_lib_path) + extraPath;
        loadDir(extraPathDir.c_str(), extension, loader);
    }

    return 0;
}

int CoreOption::load(const char* searchPath[], const char* extraPath, const char* extension,
    CoreOptionContextLoader loader, void* context) {
    int path;

    /* load normal search path */
    path = 0;
    while (searchPath[path][0] != '\0') {
        loadDir(searchPath[path], extension, loader, context);
        path++;
    }
    /* load from extra path */
    if (extra_lib_path[0] != '\0') {
        std::string extraPathDir = std::string(extra_lib_path) + extraPath;
        loadDir(extraPathDir.c_str(), extension, loader, context);
    }

    return 0;
}
