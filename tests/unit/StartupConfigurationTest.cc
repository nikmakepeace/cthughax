#include "Configuration.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

class ScopedEnvironmentVariable {
    std::string name;
    std::string previousValue;
    int hadPreviousValue;

public:
    ScopedEnvironmentVariable(const char* name_, const char* value)
        : name(name_)
        , previousValue()
        , hadPreviousValue(0) {
        const char* previous = getenv(name.c_str());
        if (previous != NULL) {
            previousValue = previous;
            hadPreviousValue = 1;
        }
        setenv(name.c_str(), value, 1);
    }

    ~ScopedEnvironmentVariable() {
        if (hadPreviousValue)
            setenv(name.c_str(), previousValue.c_str(), 1);
        else
            unsetenv(name.c_str());
    }
};

static std::string makeTempDirectory() {
    char path[] = "/tmp/cthugha-startup-config-XXXXXX";
    char* result = mkdtemp(path);
    assert(result != NULL);
    return result;
}

static void writeFile(const std::string& path, const std::string& text) {
    std::ofstream file(path.c_str());
    assert(file.good());
    file << text;
}

static void removeFile(const std::string& path) {
    unlink(path.c_str());
}

static void removeDirectory(const std::string& path) {
    rmdir(path.c_str());
}

static int indexOfFile(const std::vector<std::string>& files,
    const std::string& path) {
    for (int i = 0; i < int(files.size()); i++) {
        if (files[i] == path)
            return i;
    }

    return -1;
}

static void testBuildStartupConfigUsesRealFilesHomeEnvironmentAndCommandLine() {
    std::string home = makeTempDirectory();
    writeFile(home + "/.cthugha.auto",
        "cthugha.flame: auto-flame\n"
        "cthugha.verbose: 2\n");
    writeFile(home + "/.cthugha.ini",
        "cthugha.flame: home-flame\n"
        "cthugha.wave: home-wave\n");
    writeFile(home + "/.cthugha.continue",
        "cthugha.flame: continue-flame\n"
        "cthugha.palette: continue-palette\n");

    ScopedEnvironmentVariable scopedHome("HOME", home.c_str());
    ScopedEnvironmentVariable scopedVerbose("CTH_VERBOSE", "5");

    char arg0[] = "cthugha";
    char arg1[] = "--flame";
    char arg2[] = "cli-flame";
    char* argv[] = { arg0, arg1, arg2 };
    ConfigBuildResult result = buildStartupConfig(3, argv);

    assert(result.ok());
    assert(result.config.scene.flame == "cli-flame");
    assert(result.config.scene.wave == "home-wave");
    assert(result.config.scene.palette == "continue-palette");
    assert(result.config.logging.verbosity == 5);
    assert(result.config.paths.continuationIniFile == home + "/.cthugha.continue");
    assert(result.config.paths.iniFiles.size() >= 4);
    int autoIndex = indexOfFile(
        result.config.paths.iniFiles, home + "/.cthugha.auto");
    int homeIndex = indexOfFile(
        result.config.paths.iniFiles, home + "/.cthugha.ini");
    int continuationIndex = indexOfFile(
        result.config.paths.iniFiles, home + "/.cthugha.continue");
    assert(autoIndex >= 0);
    assert(homeIndex == autoIndex + 1);
    assert(continuationIndex > homeIndex);
    assert(result.config.paths.iniFiles[result.config.paths.iniFiles.size() - 1]
        == home + "/.cthugha.continue");

    removeFile(home + "/.cthugha.auto");
    removeFile(home + "/.cthugha.ini");
    removeFile(home + "/.cthugha.continue");
    removeDirectory(home);
}

static void testBuildStartupConfigRequiresExplicitIniFile() {
    std::string home = makeTempDirectory();
    ScopedEnvironmentVariable scopedHome("HOME", home.c_str());

    char arg0[] = "cthugha";
    char arg1[] = "--ini-file";
    char arg2[] = "/tmp/cthugha-missing-required.ini";
    char* argv[] = { arg0, arg1, arg2 };
    ConfigBuildResult result = buildStartupConfig(3, argv);

    assert(!result.ok());
    assert(!result.diagnostics.empty());
    assert(result.diagnostics[0].severity == ConfigDiagnosticError);
    assert(result.diagnostics[0].source == "/tmp/cthugha-missing-required.ini");
    assert(result.diagnostics[0].key == "ini-file");

    removeDirectory(home);
}

int main() {
    assert(configArgumentsFromArgv(0, NULL).empty());
    testBuildStartupConfigUsesRealFilesHomeEnvironmentAndCommandLine();
    testBuildStartupConfigRequiresExplicitIniFile();
    return 0;
}
