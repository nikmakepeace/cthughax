// Scene wave-object catalog loading.

#include "SceneWaveObjectCatalogLoader.h"

#include "config.h"

#include "Configuration.h"
#include "ProcessServices.h"
#include "SceneWaveObjectCatalog.h"
#include "WaveObject.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <strings.h>
#include <vector>

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

namespace {

static const char* objectPath[] = { "./", "./resources/obj/",
    CTH_LIBDIR "/obj/", "" };

static int compareFeatureFileNames(
    const std::string& left, const std::string& right) {
    int folded = strcasecmp(left.c_str(), right.c_str());

    return folded ? folded < 0 : left < right;
}

static int hasExtension(const std::string& name, const char* extension) {
    size_t extensionLen = std::strlen(extension);
    if (name.size() < extensionLen)
        return 0;

    return strcasecmp(name.c_str() + name.size() - extensionLen, extension) == 0;
}

static std::string featureNameForFile(
    const std::string& name, const char* extension) {
    size_t extensionLen = std::strlen(extension);
    for (size_t i = 0; i < name.size(); i++) {
        if (strncasecmp(name.c_str() + i, extension, extensionLen) == 0)
            return name.substr(0, i);
    }

    return name;
}

static int catalogHasName(
    const SceneWaveObjectCatalog& catalog, const char* name) {
    for (int i = 0; i < catalog.entryCount(); i++) {
        if (strcasecmp(catalog.nameAt(i), name) == 0)
            return 1;
    }

    return 0;
}

static void loadSceneWaveObjectDirectory(SceneWaveObjectCatalog& catalog,
    const char* dir, const char* extension, LogSink& log) {
    DIR* directory;
    struct dirent* entry;
    std::vector<std::string> names;

    if ((directory = opendir(dir)) == NULL)
        return;

    while ((entry = readdir(directory)) != NULL) {
        if (hasExtension(entry->d_name, extension))
            names.push_back(entry->d_name);
    }
    closedir(directory);

    std::sort(names.begin(), names.end(), compareFeatureFileNames);

    for (std::vector<std::string>::const_iterator it = names.begin();
         it != names.end(); ++it) {
        std::string totalName = std::string(dir) + *it;
        std::string featureName = featureNameForFile(*it, extension);

        if (featureName.empty()) {
            log.debug("skipping empty feature name: %s\n", totalName.c_str());
            continue;
        }

        if (catalogHasName(catalog, featureName.c_str())) {
            log.debug("already loaded: %s\n", totalName.c_str());
            continue;
        }

        log.debug("loading: %s", totalName.c_str());
        FILE* file = std::fopen(totalName.c_str(), "r");
        if (file == NULL) {
            log.errorErrno(errno, "Can not open wave object `%s'.",
                totalName.c_str());
            continue;
        }

        WObject* object = read_wave_object(file, featureName.c_str());
        std::fclose(file);

        if (object != NULL) {
            catalog.addChoice(featureName.c_str(), object, 1);
            delete[] object;
            log.debug(" ... OK\n");
        }
    }
}

}

int loadSceneWaveObjectCatalog(SceneWaveObjectCatalog& catalog,
    const PathConfig& pathConfig, int fileObjectsEnabled, LogSink& log) {
    catalog.clear();
    catalog.addChoice("bigH", builtInWaveObjectBigH(), 1);

    if (!fileObjectsEnabled)
        return 0;

    log.info("  loading 3-D objects...");
    for (int path = 0; objectPath[path][0] != '\0'; path++)
        loadSceneWaveObjectDirectory(catalog, objectPath[path], ".obj", log);

    if (!pathConfig.extraLibraryPath.empty()) {
        std::string extraPathDir = pathConfig.extraLibraryPath + "/obj/";
        loadSceneWaveObjectDirectory(catalog, extraPathDir.c_str(), ".obj", log);
    }

    log.info("\n  number of 3-D objects: %d\n", catalog.entryCount());
    return 0;
}
