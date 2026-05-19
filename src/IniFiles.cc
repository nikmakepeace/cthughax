#include "cthugha.h"
#include "options.h"
#include "Sound.h"
#include "display.h"
#include "translate.h"
#include "information.h"
#include "display.h"
#include "CDPlayer.h"
#include "keys.h"
#include "AutoChanger.h"
#include "SoundProcess.h"
#include "CthughaBuffer.h"

#include <unistd.h>
#include <ctype.h>

char extra_lib_path[PATH_MAX] = ""; /* extra path to search for
                                       pcx, tab, map and ini */
char ini_file_override[PATH_MAX] = "";

FILE* ini_file = NULL; /* the currently open ini-file */
static int ini_nr = -1;
static char ini_file_path[PATH_MAX] = "";

/*
 * create the name of an ini file
 */
char* ini_file_name(int ini_nr) {
    static char fname[PATH_MAX];
    char* var;

    switch (ini_nr) {
    case 0:
        return (CTH_LIBDIR "/cthugha.ini");
    case -1:
    case 1:
        if ((var = getenv("HOME")) == NULL)
            return NULL;
        strncpy(fname, var, PATH_MAX);
        strncat(fname, "/.cthugha.auto", PATH_MAX);
        return (fname);
    case 2:
        if ((var = getenv("HOME")) == NULL)
            return NULL;
        strncpy(fname, var, PATH_MAX);
        strncat(fname, "/.cthugha.ini", PATH_MAX);
        return (fname);
    case 3:
        return ("./cthugha.ini");
        break;
    case 4:
        if (extra_lib_path[0] == '\0')
            return NULL;
        strncpy(fname, extra_lib_path, PATH_MAX);
        strncat(fname, "cthugha.ini", PATH_MAX);
        return (fname);
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
    char* fname;

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

        if (!CoreOption::isIniEntry(entry)) {
            CTH_WARN("Unknown ini directive `cthugha.%s' in `%s' line %d.\n",
                entry, ini_file_path, line_nr);
        }
    }
}

//
// get a ini entry from the current in file
//
int getini(const char* entry, char* value) {
    char name[512];
    int line_nr;

    /* build up name and class */
    strcpy(name, "cthugha.");
    strncat(name, entry, 512);

    if (ini_file == NULL) {
        return get_ini_str_sys(name, value);
    }

    rewind(ini_file);

    line_nr = 0;

    while (!feof(ini_file)) {
        char line[256];
        char* linep = line;
        char* namep = name;

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

int getini(CoreOption& opt) {
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
int read_ini() {
    char str[256];
    struct option* opt;

    open_ini_start();
    while (open_ini_file() == 0) {

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

        CoreOption::getIniInitials();
        warn_unknown_ini_entries();

        if (ini_file)
            fclose(ini_file);
    }
    return 0;
}

/*
 * read the usage from the ini files
 */
int read_ini_usage() {

    open_ini_start();
    while (open_ini_file() == 0) {

        CoreOption::getIniUsages();
        CoreOption::getHotIni();

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
static int move(char* src, char* dst) {

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
    char* fname = ini_file_name(-1);
    char fname_dst[PATH_MAX];
    FILE* f;

    strncpy(fname_dst, fname, PATH_MAX);
    strncat(fname_dst, ".old", PATH_MAX);

    /* make sure file exists */
    if ((f = fopen(fname, "a")) == NULL) {
        CTH_ERROR("Can not open ini file.");
        return 1;
    }
    fclose(f);

    if (move(fname, fname_dst))
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
        "# Core Options\n"
        "#\n");
    CoreOption::putIniInitials();

    fprintf(ini_file,
        "#\n"
        "# Timing/Automatic changer Options\n"
        "#\n");
    putini(changeWaitMin);
    putini(changeWaitRandom);
    putini(changeFireLevel);
    putini(changeQuiet);
    putini(changeMsgTime);

    putini(lock);
    putini(change_little);

    fprintf(ini_file,
        "#\n"
        "# Core Option usage options\n"
        "#\n");
    screen.putIniUsages();

    fprintf(ini_file,
        "#\n"
        "# Core Option 'Hot' options\n"
        "#\n");
    CoreOption::putHotIni();

    /*
     * copy old settings
     */
    if ((f = fopen(fname_dst, "r")) == NULL) {
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
