#include "cthugha.h"
#include "options.h"
#include "display.h"
#include "TranslationOptions.h"
#include "information.h"
#include "display.h"
#include "EffectControlIni.h"
#include "keys.h"
#include "AutoChanger.h"
#include "AudioProcessor.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "VideoDirector.h"

#include <unistd.h>
#include <ctype.h>
#include <string>
#include <vector>

char extra_lib_path[PATH_MAX] = ""; /* extra path to search for
                                                               image, tab, map and ini */
char ini_file_override[PATH_MAX] = "";

FILE* ini_file = NULL; /* the currently open ini-file */
static int ini_nr = -1;
static char ini_file_path[PATH_MAX] = "";

void configureIniFiles(const PathConfig& paths) {
    strncpy(extra_lib_path, paths.extraLibraryPath.c_str(), PATH_MAX);
    extra_lib_path[PATH_MAX - 1] = '\0';
    strncpy(ini_file_override, paths.iniFileOverride.c_str(), PATH_MAX);
    ini_file_override[PATH_MAX - 1] = '\0';
}

static const char* continuation_ini_file_name() {
    static std::string fname;
    const char* home = getenv("HOME");

    if (home == NULL)
        return NULL;

    fname = std::string(home) + "/.cthugha.continue";
    return fname.c_str();
}

/*
 * create the name of an ini file
 */
const char* ini_file_name(int ini_nr) {
    static std::string fname;
    const char* var;

    switch (ini_nr) {
    case 0:
        return (CTH_LIBDIR "/cthugha.ini");
    case -1:
    case 1:
        if ((var = getenv("HOME")) == NULL)
            return NULL;
        fname = std::string(var) + "/.cthugha.auto";
        return fname.c_str();
    case 2:
        if ((var = getenv("HOME")) == NULL)
            return NULL;
        fname = std::string(var) + "/.cthugha.ini";
        return fname.c_str();
    case 3:
        return ("./cthugha.ini");
        break;
    case 4:
        if (extra_lib_path[0] == '\0')
            return NULL;
        fname = std::string(extra_lib_path) + "cthugha.ini";
        return fname.c_str();
    default:
        return NULL;
    }
}

int open_ini_start() {
    ini_nr = -1;
    return 0;
}

/*
 * open the next ini file
 *returns:
 * 0 -> OK, 1 -> error
 */
int open_ini_file() {
    const char* fname;

    ini_file_path[0] = '\0';

    if (ini_file_override[0] != '\0') {
        if (ini_nr == -1) {
            ini_nr = 0;
            ini_file = fopen(ini_file_override, "r");
            if (ini_file) {
                strncpy(ini_file_path, ini_file_override, PATH_MAX);
                ini_file_path[PATH_MAX - 1] = '\0';
                return 0;
            }

            CTH_ERRNO(errno, "Can not open ini file `%s'.", ini_file_override);
        }
        return 1;
    }

    for (ini_nr++; ini_nr < 6; ini_nr++) {
        if (ini_nr == 5) {
            ini_file = NULL;
            return open_ini_sys();
        }

        fname = ini_file_name(ini_nr);
        if (fname)
            ini_file = fopen(fname, "r");
        if (ini_file) {
            strncpy(ini_file_path, fname, PATH_MAX);
            ini_file_path[PATH_MAX - 1] = '\0';
            return 0;
        }
    }
    return 1;
}

static int ini_entry_name(const char* line, char* entry, int entry_size, int* malformed) {
    const char* linep = line;
    const char* colon;
    const char* end;
    int len;

    *malformed = 0;

    while (isspace(*linep))
        linep++;

    switch (*linep) {
    case '#':
    case '!':
    case '\0':
        return 0;
    }

    if (strncasecmp(linep, "cthugha.", 8) != 0)
        return 0;

    linep += 8;
    colon = strchr(linep, ':');
    if (colon == NULL) {
        *malformed = 1;
        strncpy(entry, linep, entry_size);
        entry[entry_size - 1] = '\0';
        return 1;
    }

    end = colon;
    while ((end > linep) && isspace(end[-1]))
        end--;

    len = end - linep;
    if (len >= entry_size)
        len = entry_size - 1;

    strncpy(entry, linep, len);
    entry[len] = '\0';

    return 1;
}

static const char* canonical_long_option_name(const struct option* opt) {
    struct option* other;

    for (other = long_options; other->name != NULL; other++) {
        if ((other->flag == opt->flag) && (other->val == opt->val)
            && (other->has_arg == opt->has_arg)) {
            return other->name;
        }
    }

    return opt->name;
}

static int is_long_option_ini_entry(const char* entry, const char** canonical) {
    struct option* opt;

    for (opt = long_options; opt->name != NULL; opt++) {
        if (strcasecmp(entry, opt->name) == 0) {
            *canonical = canonical_long_option_name(opt);
            return strcasecmp(entry, *canonical) == 0;
        }
    }

    *canonical = NULL;
    return 0;
}

static void warn_unknown_ini_entries() {
    int line_nr = 0;

    if (ini_file == NULL)
        return;

    rewind(ini_file);

    while (!feof(ini_file)) {
        char line[256];
        char entry[256];
        const char* canonical;
        int malformed;

        line_nr++;
        line[0] = '\0';
        fgets(line, 256, ini_file);

        if (!ini_entry_name(line, entry, sizeof(entry), &malformed))
            continue;

        if (malformed) {
            CTH_WARN("Malformed ini directive `%s' in `%s' line %d.\n",
                entry, ini_file_path, line_nr);
            continue;
        }

        if (!is_long_option_ini_entry(entry, &canonical)) {
            if (canonical) {
                CTH_WARN("Unsupported ini directive `cthugha.%s' in `%s' line %d; use `cthugha.%s' instead.\n",
                    entry, ini_file_path, line_nr, canonical);
                continue;
            }
        } else {
            continue;
        }

        if (!effectControlIsIniEntry(entry)) {
            CTH_WARN("Unknown ini directive `cthugha.%s' in `%s' line %d.\n",
                entry, ini_file_path, line_nr);
        }
    }
}

//
// get a ini entry from the current in file
//
int getini(const char* entry, char* value) {
    std::string name;
    int line_nr;

    /* build up name and class */
    name = std::string("cthugha.") + entry;

    if (ini_file == NULL) {
        return get_ini_str_sys(name.c_str(), value);
    }

    rewind(ini_file);

    line_nr = 0;

    while (!feof(ini_file)) {
        char line[256];
        char* linep = line;
        const char* namep = name.c_str();

        line_nr++;
        line[0] = '\0';

        fgets(line, 256, ini_file); /* get one line */

        while (isspace(*linep)) /* skip whitespace */
            linep++;

        switch (*linep) {
        case '#':
        case '!':
        case '\0': /* comment or empty */
            break;
        default:
            /* compare word by word */
            while (((toupper(*namep) == toupper(*linep)) || (*linep == '?')) && (*namep != '\0')) {
                if (*linep == '?') { /* wildcard */
                    while ((*namep != '.') && (*namep != '\0'))
                        namep++;
                    linep++;
                } else { /* compare word */
                    namep++, linep++;
                }
            }

            while (isspace(*linep)) /* skip whitespace */
                linep++;

            if ((*namep == '\0') && (*linep == ':')) { /* found entry */
                linep++;
                while (isspace(*linep)) /* skip whitespace */
                    linep++;

                if ((*linep != '\0') && (*linep != '!') && (*linep != '#')) { /* value is given */
                    // copy everything till end of line or beginn of comment
                    while ((*linep != '\0') && (*linep != '!') && (*linep != '#')) {
                        *value = *linep;
                        linep++;
                        value++;
                    }
                    *value = '\0'; // set the last char to 0
                    value--; // might be ! or # and step one left
                    while (isspace(*value)) { // remove all trailing white spaces
                        *value = '\0';
                        value--;
                    }
                    return 0;
                } else /* no value */
                    return 1;
            }
        }
    }

    return 1;
}
int getini(const char* entry, int* value) {
    char* pos;
    char str[512];

    if (getini(entry, str)) // get the entry
        return 1;

    /* try to read as number */
    int tvalue = strtol(str, &pos, 0);
    if (str == pos) { // not a number
        CTH_ERROR("Not a number `%s' for entry `%s'.\n", str, entry);
        return 1;
    }

    *value = tvalue;

    return 0;
}
int getini_yesno(const char* entry, int* value) {
    char str[512];

    if (getini(entry, str))
        return 1;

    if (!strncasecmp("yes", str, 3))
        *value = 1;
    else if (!strncasecmp("on", str, 2))
        *value = 1;
    else if (!strncasecmp("1", str, 1))
        *value = 1;
    else if (!strncasecmp("no", str, 2))
        *value = 0;
    else if (!strncasecmp("off", str, 3))
        *value = 0;
    else if (!strncasecmp("0", str, 1))
        *value = 0;
    else if (!strncasecmp("disable", str, 7))
        *value = 2;
    else {
        CTH_ERROR("Illegal value `%s' for entry `%s'.\n", str, entry);
    }

    return 0;
}

int getini(EffectControl& opt) {
    char str[512];

    if (getini(opt.name(), str))
        return 1;

    opt.setInitialEntry(str);
    return 0;
}

/*
 * write a ini entry
 */
int putini(const char* entry, const char* value) {
    fprintf(ini_file, "Cthugha.%s: %s\n", entry, value);
    return 0;
}
int putini(const Option& opt) { return putini(opt.name(), opt.text()); }

/*
 *  Read settings from ini-file's
 */
static void read_current_ini_initials() {
    char str[256];
    struct option* opt;

    for (opt = long_options; opt->name != NULL; opt++) {
        if (strcasecmp(opt->name, canonical_long_option_name(opt)) != 0)
            continue;

        if ((opt->flag == 0) || // no variable, must pass it to do_param
            (opt->has_arg != 0)) { // has argument, must pass it to do_param
            if (getini(opt->name, str) == 0) { // there is an entry in the ini-file
                if ((strcasecmp(opt->name, "ini-file") == 0) && (ini_file != NULL)) {
                    CTH_WARN("Ignoring `cthugha.ini-file' in `%s'; ini-file chaining is not supported. Use --ini-file on the command line.\n",
                        ini_file_path);
                    continue;
                }
                do_param(opt->val, atoi(str), str);
            }
        } else { // variable given, we must take care
            getini_yesno(opt->name, opt->flag);
        }
    }

    warn_unknown_ini_entries();
}

static int read_continuation_ini() {
    const char* fname = continuation_ini_file_name();
    if (fname == NULL)
        return 0;

    ini_file = fopen(fname, "r");
    if (ini_file == NULL)
        return 0;

    strncpy(ini_file_path, fname, PATH_MAX);
    ini_file_path[PATH_MAX - 1] = '\0';

    CTH_INFO("Loading continuation state from `%s'.\n", ini_file_path);
    read_current_ini_initials();

    fclose(ini_file);
    ini_file = NULL;

    if (unlink(fname) != 0) {
        CTH_ERRNO(errno, "Can not remove continuation ini `%s'.", fname);
        return 0;
    }

    CTH_DEBUG("Removed continuation ini `%s'.\n", fname);
    return 0;
}

static int read_ini_files_from_config(const PathConfig& paths,
    int includeContinuation, int removeContinuation) {
    for (std::vector<std::string>::const_iterator it = paths.iniFiles.begin();
         it != paths.iniFiles.end(); ++it) {
        const std::string& path = *it;
        int isContinuation = !paths.continuationIniFile.empty()
            && (path == paths.continuationIniFile);
        int required = !paths.iniFileOverride.empty()
            && (path == paths.iniFileOverride);

        if (isContinuation && !includeContinuation)
            continue;

        ini_file = fopen(path.c_str(), "r");
        if (ini_file == NULL) {
            if (required)
                CTH_ERRNO(errno, "Can not open ini file `%s'.", path.c_str());
            continue;
        }

        strncpy(ini_file_path, path.c_str(), PATH_MAX);
        ini_file_path[PATH_MAX - 1] = '\0';

        if (isContinuation)
            CTH_INFO("Loading continuation state from `%s'.\n", ini_file_path);

        read_current_ini_initials();

        fclose(ini_file);
        ini_file = NULL;

        if (isContinuation && removeContinuation) {
            if (unlink(path.c_str()) != 0) {
                CTH_ERRNO(errno, "Can not remove continuation ini `%s'.",
                    path.c_str());
            } else {
                CTH_DEBUG("Removed continuation ini `%s'.\n", path.c_str());
            }
        }
    }

    return 0;
}

int read_ini(const PathConfig& paths) {
    return read_ini_files_from_config(paths, 1, 1);
}

int read_ini() {
    open_ini_start();
    while (open_ini_file() == 0) {
        read_current_ini_initials();

        if (ini_file)
            fclose(ini_file);
    }

    return read_continuation_ini();
}

/*
 * read per-entry usage flags and preset slots after effect catalogs exist
 */
int read_effect_control_usage_and_presets(const PathConfig& paths) {
    for (std::vector<std::string>::const_iterator it = paths.iniFiles.begin();
         it != paths.iniFiles.end(); ++it) {
        if (!paths.continuationIniFile.empty()
            && (*it == paths.continuationIniFile))
            continue;

        ini_file = fopen(it->c_str(), "r");
        if (ini_file == NULL)
            continue;

        strncpy(ini_file_path, it->c_str(), PATH_MAX);
        ini_file_path[PATH_MAX - 1] = '\0';

        effectControlGetIniUsages();
        effectControlGetPresetIni();

        fclose(ini_file);
        ini_file = NULL;
    }

    return 0;
}

int read_effect_control_usage_and_presets() {

    open_ini_start();
    while (open_ini_file() == 0) {

        effectControlGetIniUsages();
        effectControlGetPresetIni();

        /* close the ini-file */
        if (ini_file)
            fclose(ini_file);
    }
    return 0;
}

/*
 * move old ini-file to backup file
 * ~/.cthugha.auto -> ~/.cthugha.auto.old
 */
static int move(const char* src, const char* dst) {

    unlink(dst);

    if (link(src, dst)) {
        CTH_ERRNO(errno, "Can not make backup of %s", src);
        return 1;
    }

    if (unlink(src)) {
        CTH_ERRNO(errno, "Can not remove original %s", src);
        return 1;
    }
    return 0;
}

static int is_in_ini(char* line) {
    char* ptr;
    int len;

    /* only lines with values are interresting */
    if ((ptr = strchr(line, ':')) == NULL)
        return 1;

    len = ptr - line;

    rewind(ini_file);
    while (!feof(ini_file)) {
        char line2[256];

        fgets(line2, 256, ini_file);
        if (strncmp(line, line2, len) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 *  Create the automatic ini-file
 */
int write_ini() {
    const char* fname = ini_file_name(-1);
    std::string fnameDst;
    FILE* f;

    fnameDst = std::string(fname) + ".old";

    /* make sure file exists */
    if ((f = fopen(fname, "a")) == NULL) {
        CTH_ERROR("Can not open ini file.");
        return 1;
    }
    fclose(f);

    if (move(fname, fnameDst.c_str()))
        return 1;

    if ((ini_file = fopen(fname, "w+")) == NULL)
        return 1;

    /* write header */
    fprintf(ini_file,
        "#\n"
        "# .cthugha.auto\n"
        "#\n"
        "# This file was created automatically, please do not edit.\n"
        "#\n");

    fprintf(ini_file,
        "#\n"
        "# Effect Controls\n"
        "#\n");
    effectControlPutIniInitials();
    putini(audioProcessing);

    fprintf(ini_file,
        "#\n"
        "# Timing/Automatic changer Options\n"
        "#\n");
    putini(changeWaitMin);
    putini(changeWaitRandom);
    putini(changeCumulativeFireLevel);
    putini(changeQuiet);
    putini(changeMsgTime);

    putini(lock);
    putini(change_little);
    putini(showFPS);

    fprintf(ini_file,
        "#\n"
        "# Effect Control usage options\n"
        "#\n");
    effectControlPutIniUsages();

    fprintf(ini_file,
        "#\n"
        "# Effect Control preset slots\n"
        "#\n");
    effectControlPutPresetIni();

    /*
     * copy old settings
     */
    if ((f = fopen(fnameDst.c_str(), "r")) == NULL) {
        CTH_ERROR("Can not open backup file");
        return 1;
    }

    /* check for all files, if they are in the ini file */
    while (!feof(f)) {
        char line[PATH_MAX];

        fgets(line, PATH_MAX, f);
        if (!is_in_ini(line)) {
            fseek(ini_file, 0, SEEK_END);
            fputs(line, ini_file);
        }
    }

    fclose(f);
    fclose(ini_file);

    return 0;
}

int write_continuation_ini() {
    const char* fname = continuation_ini_file_name();
    if (fname == NULL) {
        CTH_ERROR("Can not create continuation ini: HOME is not set.\n");
        return 1;
    }

    std::string tmpName = std::string(fname) + ".tmp";
    FILE* previous_ini_file = ini_file;
    FILE* out = fopen(tmpName.c_str(), "w");
    if (out == NULL) {
        CTH_ERRNO(errno, "Can not create continuation ini `%s'.", tmpName.c_str());
        return 1;
    }

    ini_file = out;
    fprintf(ini_file,
        "#\n"
        "# .cthugha.continue\n"
        "#\n"
        "# One-shot stop-and-continue state. Cthugha deletes this file after startup.\n"
        "#\n"
        "# Effect Controls\n"
        "#\n");
    effectControlPutIniInitials();
    putini(audioProcessing);
    putini(showFPS);

    int write_error = ferror(ini_file);
    int close_error = fclose(ini_file);
    ini_file = previous_ini_file;

    if (write_error || close_error) {
        unlink(tmpName.c_str());
        CTH_ERROR("Can not write continuation ini `%s'.\n", tmpName.c_str());
        return 1;
    }

    if (rename(tmpName.c_str(), fname) != 0) {
        unlink(tmpName.c_str());
        CTH_ERRNO(errno, "Can not install continuation ini `%s'.", fname);
        return 1;
    }

    CTH_INFO("Saved continuation state to `%s'.\n", fname);
    return 0;
}
