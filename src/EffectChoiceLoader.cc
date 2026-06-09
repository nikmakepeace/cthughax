// Filesystem/catalog loader for Effect choices.

#include "EffectChoiceLoader.h"

#include "cthugha.h"
#include "Configuration.h"

#include <string>

// autoconf suggests this
#if HAVE_DIRENT_H
#include <dirent.h>
#else
#define dirent direct
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

static int compareFeatureFileNames(const void* a, const void* b) {
    const char* left = *(const char* const*)a;
    const char* right = *(const char* const*)b;
    int folded = strcasecmp(left, right);

    return folded ? folded : strcmp(left, right);
}

static int hasExtension(const char* name, const char* extension) {
    size_t name_len = strlen(name);
    size_t extension_len = strlen(extension);

    if (name_len < extension_len)
        return 0;

    return strcasecmp(name + name_len - extension_len, extension) == 0;
}

static EffectChoice* loadEntry(const char* name, char* total_name, const char* dir,
    EffectChoiceLoader loader) {

    FILE* file = fopen(total_name, "r");

    CTH_DEBUG("loading: %s", total_name);

    /* now do the loading */
    if (file != NULL) {
        EffectChoice* entry;

        /* file was opened successfully - now read it */
        if ((entry = (*loader)(file, name, dir, total_name)) != NULL) {
            CTH_DEBUG(" ... OK\n");
        }

        fclose(file);

        return entry;

    } else { /* file/pipe could not be opened */
        CTH_DEBUG(" ... x\n");
        return NULL;
    }
}

static EffectChoice* loadEntry(const char* name, char* total_name, const char* dir,
    EffectChoiceContextLoader loader, void* context) {

    FILE* file = fopen(total_name, "r");

    CTH_DEBUG("loading: %s", total_name);

    /* now do the loading */
    if (file != NULL) {
        EffectChoice* entry;

        /* file was opened successfully - now read it */
        if ((entry = (*loader)(file, name, dir, total_name, context)) != NULL) {
            CTH_DEBUG(" ... OK\n");
        }

        fclose(file);

        return entry;

    } else { /* file/pipe could not be opened */
        CTH_DEBUG(" ... x\n");
        return NULL;
    }
}

static void loadDir(EffectControl& option, const char* dir, const char* extension,
    EffectChoiceLoader loader) {
    char total_name[PATH_MAX];
    char feat_name[PATH_MAX];
    DIR* directory;
    struct dirent* entry;
    EffectChoice* fe;
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

            if (option.defined(feat_name)) {
                CTH_DEBUG("already loaded: %s\n", total_name);
            } else if ((fe = loadEntry(feat_name, total_name, dir, loader)) != NULL)
                option.add(fe);

            delete[] names[i];
        }
        free(names);
    }
}

static void loadDir(EffectControl& option, const char* dir, const char* extension,
    EffectChoiceContextLoader loader, void* context) {
    char total_name[PATH_MAX];
    char feat_name[PATH_MAX];
    DIR* directory;
    struct dirent* entry;
    EffectChoice* fe;
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

            if (option.defined(feat_name)) {
                CTH_DEBUG("already loaded: %s\n", total_name);
            } else if ((fe = loadEntry(feat_name, total_name, dir, loader, context)) != NULL)
                option.add(fe);

            delete[] names[i];
        }
        free(names);
    }
}

int loadEffectChoices(EffectControl& option, const PathConfig& pathConfig,
    const char* searchPath[], const char* extraPath, const char* extension,
    EffectChoiceLoader loader) {
    int path;

    /* load normal search path */
    path = 0;
    while (searchPath[path][0] != '\0') {
        loadDir(option, searchPath[path], extension, loader);
        path++;
    }
    /* load from extra path */
    if (!pathConfig.extraLibraryPath.empty()) {
        std::string extraPathDir = pathConfig.extraLibraryPath + extraPath;
        loadDir(option, extraPathDir.c_str(), extension, loader);
    }

    return 0;
}

int loadEffectChoices(EffectControl& option, const PathConfig& pathConfig,
    const char* searchPath[], const char* extraPath, const char* extension,
    EffectChoiceContextLoader loader, void* context) {
    int path;

    /* load normal search path */
    path = 0;
    while (searchPath[path][0] != '\0') {
        loadDir(option, searchPath[path], extension, loader, context);
        path++;
    }
    /* load from extra path */
    if (!pathConfig.extraLibraryPath.empty()) {
        std::string extraPathDir = pathConfig.extraLibraryPath + extraPath;
        loadDir(option, extraPathDir.c_str(), extension, loader, context);
    }

    return 0;
}
