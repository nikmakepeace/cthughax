// Scene image catalog loading.

#include "SceneImageCatalogLoader.h"

#include "config.h"

#include "Configuration.h"
#include "IndexedImage.h"
#include "ProcessServices.h"
#include "SceneImageCatalog.h"
#include "pcx.h"
#include "png.h"

#include <algorithm>
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

typedef IndexedImage* (*IndexedImageLoader)(
    FILE*, const char*, const char*, const char*, const ImageLoadTarget&);

struct SceneImageFileFormat {
    const char* extension;
    IndexedImageLoader loader;
};

static const char* imagePath[] = { "./", "./resources/img/",
    CTH_LIBDIR "/img/", "" };

static const SceneImageFileFormat sceneImageFileFormats[] = {
    { ".pcx", read_pcx_indexed_image },
    { ".png", read_png_indexed_image },
    { 0, 0 }
};

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
    const SceneImageCatalog& catalog, const char* name) {
    for (int i = 0; i < catalog.entryCount(); i++) {
        if (strcasecmp(catalog.nameAt(i), name) == 0)
            return 1;
    }

    return 0;
}

static void loadSceneImageDirectory(SceneImageCatalog& catalog,
    const char* dir, const SceneImageFileFormat& format,
    const ImageLoadTarget& target, LogSink& log) {
    DIR* directory;
    struct dirent* entry;
    std::vector<std::string> names;

    if ((directory = opendir(dir)) == NULL)
        return;

    while ((entry = readdir(directory)) != NULL) {
        if (hasExtension(entry->d_name, format.extension))
            names.push_back(entry->d_name);
    }
    closedir(directory);

    std::sort(names.begin(), names.end(), compareFeatureFileNames);

    for (std::vector<std::string>::const_iterator it = names.begin();
         it != names.end(); ++it) {
        std::string totalName = std::string(dir) + *it;
        std::string featureName = featureNameForFile(*it, format.extension);

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
            log.debug(" ... x\n");
            continue;
        }

        IndexedImage* image = format.loader(file, featureName.c_str(),
            dir, totalName.c_str(), target);
        std::fclose(file);

        if (image != NULL) {
            catalog.addChoice(featureName.c_str(), image, 1);
            delete image;
            log.debug(" ... OK\n");
        }
    }
}

}

int loadSceneImageCatalog(SceneImageCatalog& catalog,
    const PathConfig& pathConfig, int fileImagesEnabled, int targetWidth,
    int targetHeight, LogSink& log) {
    catalog.clear();
    catalog.addChoice("none", NULL, 1);

    if (!fileImagesEnabled)
        return 0;

    ImageLoadTarget target(targetWidth, targetHeight);
    log.info("  loading Scene image files...\n");
    for (const SceneImageFileFormat* format = sceneImageFileFormats;
         format->extension != 0; format++) {
        for (int path = 0; imagePath[path][0] != '\0'; path++)
            loadSceneImageDirectory(catalog, imagePath[path], *format, target, log);

        if (!pathConfig.extraLibraryPath.empty()) {
            std::string extraPathDir = pathConfig.extraLibraryPath + "/img/";
            loadSceneImageDirectory(catalog, extraPathDir.c_str(), *format,
                target, log);
        }
    }

    log.info("  number of Scene image files: %d\n", catalog.entryCount());
    return 0;
}
