#include "IniFiles.h"

#include "cthugha.h"
#include "EffectControlIni.h"
#include "AutoChanger.h"
#include "AudioProcessor.h"
#include "CthughaDisplay.h"
#include "VideoDirector.h"

#include <ctype.h>
#include <string>
#include <unistd.h>

static FILE* ini_file = NULL;

static const char* home_ini_file_name(const char* fileName) {
    static std::string fname;
    const char* home = getenv("HOME");

    if (home == NULL)
        return NULL;

    fname = std::string(home) + "/" + fileName;
    return fname.c_str();
}

static const char* automatic_ini_file_name() {
    return home_ini_file_name(".cthugha.auto");
}

static const char* continuation_ini_file_name() {
    return home_ini_file_name(".cthugha.continue");
}

int getini(const char* entry, char* value) {
    std::string name;
    int line_nr;

    name = std::string("cthugha.") + entry;

    if (ini_file == NULL)
        return 1;

    rewind(ini_file);

    line_nr = 0;

    while (!feof(ini_file)) {
        char line[256];
        char* linep = line;
        const char* namep = name.c_str();

        line_nr++;
        line[0] = '\0';

        fgets(line, 256, ini_file);

        while (isspace(*linep))
            linep++;

        switch (*linep) {
        case '#':
        case '!':
        case '\0':
            break;
        default:
            while (((toupper(*namep) == toupper(*linep)) || (*linep == '?'))
                && (*namep != '\0')) {
                if (*linep == '?') {
                    while ((*namep != '.') && (*namep != '\0'))
                        namep++;
                    linep++;
                } else {
                    namep++, linep++;
                }
            }

            while (isspace(*linep))
                linep++;

            if ((*namep == '\0') && (*linep == ':')) {
                linep++;
                while (isspace(*linep))
                    linep++;

                if ((*linep != '\0') && (*linep != '!') && (*linep != '#')) {
                    while ((*linep != '\0') && (*linep != '!')
                        && (*linep != '#')) {
                        *value = *linep;
                        linep++;
                        value++;
                    }
                    *value = '\0';
                    value--;
                    while (isspace(*value)) {
                        *value = '\0';
                        value--;
                    }
                    return 0;
                } else {
                    return 1;
                }
            }
        }
    }

    return 1;
}

int getini(const char* entry, int* value) {
    char* pos;
    char str[512];

    if (getini(entry, str))
        return 1;

    int tvalue = strtol(str, &pos, 0);
    if (str == pos) {
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
    else
        CTH_ERROR("Illegal value `%s' for entry `%s'.\n", str, entry);

    return 0;
}

int putini(const char* entry, const char* value) {
    fprintf(ini_file, "Cthugha.%s: %s\n", entry, value);
    return 0;
}

int putini(const Option& opt) {
    return putini(opt.name(), opt.text());
}

int remove_continuation_ini(const PathConfig& paths) {
    if (paths.continuationIniFile.empty())
        return 0;

    if (unlink(paths.continuationIniFile.c_str()) == 0) {
        CTH_DEBUG("Removed continuation ini `%s'.\n",
            paths.continuationIniFile.c_str());
        return 0;
    }

    if (errno != ENOENT)
        CTH_ERRNO(errno, "Can not remove continuation ini `%s'.",
            paths.continuationIniFile.c_str());

    return 0;
}

static int move_ini_file(const char* src, const char* dst) {
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

    if ((ptr = strchr(line, ':')) == NULL)
        return 1;

    len = ptr - line;

    rewind(ini_file);
    while (!feof(ini_file)) {
        char line2[256];

        fgets(line2, 256, ini_file);
        if (strncmp(line, line2, len) == 0)
            return 1;
    }
    return 0;
}

int write_ini() {
    const char* fname = automatic_ini_file_name();
    std::string fnameDst;
    FILE* f;

    if (fname == NULL) {
        CTH_ERROR("Can not create automatic ini: HOME is not set.\n");
        return 1;
    }

    fnameDst = std::string(fname) + ".old";

    if ((f = fopen(fname, "a")) == NULL) {
        CTH_ERROR("Can not open ini file.");
        return 1;
    }
    fclose(f);

    if (move_ini_file(fname, fnameDst.c_str()))
        return 1;

    if ((ini_file = fopen(fname, "w+")) == NULL)
        return 1;

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

    if ((f = fopen(fnameDst.c_str(), "r")) == NULL) {
        CTH_ERROR("Can not open backup file");
        return 1;
    }

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
    ini_file = NULL;

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
        CTH_ERRNO(errno, "Can not create continuation ini `%s'.",
            tmpName.c_str());
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
