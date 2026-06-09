/** @file
 * Startup configuration file discovery and full startup acquisition.
 */

#include "Configuration.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#ifndef CTH_LIBDIR
#define CTH_LIBDIR ""
#endif

namespace {

static const char* KEY_PATH_EXTRA_LIBRARY = "paths.extra_library";
static const char* KEY_PATH_INI_OVERRIDE = "paths.ini_file_override";

static bool startsWith(const std::string& value, const std::string& prefix) {
    return value.compare(0, prefix.size(), prefix) == 0;
}

static std::string joinedPath(const std::string& directory,
    const std::string& fileName) {
    if (directory.empty())
        return fileName;
    if (directory[directory.size() - 1] == '/')
        return directory + fileName;
    return directory + "/" + fileName;
}

static std::string withTrailingSlash(const std::string& path) {
    if (path.empty())
        return path;
    if (path[path.size() - 1] == '/')
        return path;
    return path + "/";
}

static void addHomeFile(std::vector<std::string>& files,
    const std::string& fileName) {
    const char* home = std::getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return;

    files.push_back(joinedPath(home, fileName));
}

static bool readOptionValue(const std::vector<std::string>& args, int* index,
    const std::string& optionName, std::string* value,
    DeferredLogBuffer& diagnostics) {
    if (*index + 1 >= int(args.size())) {
        diagnostics.error("command line", optionName, "missing required argument");
        return false;
    }

    (*index)++;
    *value = args[*index];
    return true;
}

static bool readShortOptionValue(const std::vector<std::string>& args,
    int* index, const std::string& arg, std::string* value,
    DeferredLogBuffer& diagnostics) {
    if (arg.size() > 2) {
        *value = arg.substr(2);
        return true;
    }

    return readOptionValue(args, index, arg, value, diagnostics);
}

static ConfigPatch acquireBootstrapCommandLineConfig(
    const std::vector<std::string>& args, DeferredLogBuffer& diagnostics) {
    ConfigPatch patch;

    for (int i = 1; i < int(args.size()); i++) {
        const std::string& arg = args[i];

        if (arg == "--") {
            break;
        } else if (arg == "--path") {
            std::string value;
            if (readOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                    "command line");
        } else if (startsWith(arg, "--path=")) {
            patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(arg.substr(7)),
                "command line");
        } else if (arg == "--ini-file") {
            std::string value;
            if (readOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_INI_OVERRIDE, value, "command line");
        } else if (startsWith(arg, "--ini-file=")) {
            patch.set(KEY_PATH_INI_OVERRIDE, arg.substr(11), "command line");
        } else if (startsWith(arg, "-E")) {
            std::string value;
            if (readShortOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                    "command line");
        }
    }

    return patch;
}

static std::vector<std::string> startupIniFiles(
    const ConfigPatch& commandLinePatch, std::string* continuationFile) {
    std::vector<std::string> files;
    const std::string* overridePath
        = commandLinePatch.value(KEY_PATH_INI_OVERRIDE);
    const std::string* extraPath
        = commandLinePatch.value(KEY_PATH_EXTRA_LIBRARY);

    if (overridePath != NULL && !overridePath->empty()) {
        files.push_back(*overridePath);
    } else {
        std::string libDir = CTH_LIBDIR;
        if (!libDir.empty())
            files.push_back(joinedPath(libDir, "cthugha.ini"));

        addHomeFile(files, ".cthugha.auto");
        addHomeFile(files, ".cthugha.ini");
        files.push_back("./cthugha.ini");

        if (extraPath != NULL && !extraPath->empty())
            files.push_back(joinedPath(*extraPath, "cthugha.ini"));
    }

    const char* home = std::getenv("HOME");
    continuationFile->clear();
    if (home != NULL && home[0] != '\0') {
        *continuationFile = joinedPath(home, ".cthugha.continue");
        files.push_back(*continuationFile);
    }

    return files;
}

}

ConfigBuildResult buildStartupConfig(int argc, char* argv[]) {
    std::vector<std::string> args = configArgumentsFromArgv(argc, argv);
    DeferredLogBuffer bootstrapDiagnostics;

    ConfigPatch commandLinePatch
        = acquireBootstrapCommandLineConfig(args, bootstrapDiagnostics);
    std::string continuationFile;
    std::vector<std::string> iniFiles
        = startupIniFiles(commandLinePatch, &continuationFile);

    ConfigurationBuilder builder(bootstrapDiagnostics);
    builder.addDefaults();

    bool isFirst = true;
    for (std::vector<std::string>::const_iterator it = iniFiles.begin();
         it != iniFiles.end(); ++it) {
        bool optional = true;
        if (isFirst && commandLinePatch.has(KEY_PATH_INI_OVERRIDE))
            optional = false;

        builder.addIniFile(*it, optional);
        isFirst = false;
    }

    std::vector<std::string> environmentNames;
    environmentNames.push_back("CTH_VERBOSE");
#ifdef CTH_XWIN
    environmentNames.push_back("CTHUGHA_DUMP_X11_FRAMES");
    environmentNames.push_back("CTHUGHA_DUMP_X11_FRAME_LIMIT");
    environmentNames.push_back("CTHUGHA_DUMP_X11_FRAME_EVERY");
#endif
    builder.addEnvironmentVariables(environmentNames);
    builder.addCommandLine(std::move(args));

    ConfigBuildResult result = builder.build();
    result.config.paths.iniFiles = std::move(iniFiles);
    result.config.paths.continuationIniFile = std::move(continuationFile);
    return result;
}
