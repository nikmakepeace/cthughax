// Explicit startup configuration acquisition and typed config slices.

#ifndef CTHUGHA_CONFIGURATION_H
#define CTHUGHA_CONFIGURATION_H

#include "AudioTypes.h"

#include <map>
#include <string>
#include <vector>

enum ConfigDiagnosticSeverity {
    ConfigDiagnosticInfo,
    ConfigDiagnosticWarning,
    ConfigDiagnosticError
};

struct ConfigDiagnostic {
    ConfigDiagnosticSeverity severity;
    std::string source;
    std::string key;
    std::string message;

    ConfigDiagnostic();
    ConfigDiagnostic(ConfigDiagnosticSeverity severityValue,
        const std::string& sourceValue, const std::string& keyValue,
        const std::string& messageValue);
};

class DeferredLogBuffer {
    std::vector<ConfigDiagnostic> diagnosticsValue;

public:
    void add(ConfigDiagnosticSeverity severity, const std::string& source,
        const std::string& key, const std::string& message);
    void info(const std::string& source, const std::string& key,
        const std::string& message);
    void warning(const std::string& source, const std::string& key,
        const std::string& message);
    void error(const std::string& source, const std::string& key,
        const std::string& message);

    const std::vector<ConfigDiagnostic>& diagnostics() const;
    bool hasErrors() const;
    void append(const DeferredLogBuffer& other);
};

struct ConfigEntry {
    std::string key;
    std::string value;
    std::string source;

    ConfigEntry();
    ConfigEntry(const std::string& keyValue, const std::string& valueValue,
        const std::string& sourceValue);
};

class ConfigPatch {
    std::map<std::string, ConfigEntry> entriesValue;
    std::map<std::string, std::vector<ConfigEntry> > repeatedEntriesValue;

public:
    void set(const std::string& key, const std::string& value,
        const std::string& source);
    void append(const std::string& key, const std::string& value,
        const std::string& source);
    bool has(const std::string& key) const;
    const ConfigEntry* entry(const std::string& key) const;
    const std::string* value(const std::string& key) const;
    const std::map<std::string, ConfigEntry>& entries() const;
    const std::vector<ConfigEntry>* entriesFor(const std::string& key) const;
    void mergeFrom(const ConfigPatch& patch);
};

struct LoggingConfig {
    int verbosity;

    LoggingConfig();
};

struct AppConfig {
    int optionsSaveEnabled;
    int escapeKeyEnabled;
    std::string keymapFile;

    AppConfig();
};

struct PathConfig {
    std::string extraLibraryPath;
    std::string iniFileOverride;
    std::vector<std::string> iniFiles;
    std::string continuationIniFile;

    PathConfig();
};

struct CatalogConfig {
    int doubleLoadEnabled;

    CatalogConfig();
};

struct SceneConfig {
    std::string flame;
    std::string generalFlame;
    std::string wave;
    std::string waveScale;
    std::string object;
    std::string translation;
    std::string palette;
    std::string border;
    std::string flashlight;
    std::string table;
    std::string image;
    std::string presentation;

    SceneConfig();
};

struct AudioMixerInitialVolumeConfig {
    std::string name;
    int volume;
};

struct AudioConfig {
    AudioInputMode inputMode;
    std::string inputFile;
    int inputLoopEnabled;
    int sampleRateHz;
    int channels;
    AudioSampleFormat sampleFormat;
    int dspMethod;
    int dspFragments;
    int dspFragmentSize;
    int dspSyncEnabled;
    int silentEnabled;
    int minNoise;
    int mixerVolume;
    int pulseLatencyMs;
    std::string pulseServer;
    std::string outputDumpPath;
    std::string dspDevicePath;
    std::string mixerDevicePath;
    std::vector<AudioMixerInitialVolumeConfig> mixerInitialVolumes;
    int nullOutputTargetLatencyMs;
    int pulseOutputTargetLatencyMs;
    int dspOutputTargetLatencyMs;

    AudioConfig();
};

struct DisplayConfig {
    int displayMode;
    bool hasCustomDisplaySize;
    int displayWidth;
    int displayHeight;
    int bufferWidth;
    int bufferHeight;
    bool hasCustomBufferSize;
    int maxFramesPerSecond;
    int showFpsEnabled;
    int zoomMode;
    int textOnTerm;
    int ncursesEnabled;
    std::string screenshotFilePrefix;
    int x11OverrideRedirect;
    int x11PrivateCmap;
    int x11MitShm;
    int x11RootWindow;
    int x11Fullscreen;
    int x11WindowPositionEnabled;
    int x11WindowPositionX;
    int x11WindowPositionY;
    int x11PanelEnabled;
    std::string x11FontName;

    DisplayConfig();
};

struct AutoChangeConfig {
    int quietMs;
    int waitMinMs;
    int waitRandomMs;
    int waitRandomMinimumMs;
    int cumulativeFireLevel;
    int locked;
    int changeLittle;

    AutoChangeConfig();
};

struct VisualConfig {
    int changeMessageMs;
    int quietMessageDurationMs;
    double paletteSmoothingChance;
    int paletteSmoothSeconds;
    int imageLoadingEnabled;
    std::string paletteSetFilterText;
    int paletteSetFilterCount;
    int useTranslatesEnabled;
    int useObjectsEnabled;

    VisualConfig();
};

struct MessagesConfig {
    int qotdPrefetchTimeoutMs;
    std::string qotdServer;
    std::string qotdPort;

    MessagesConfig();
};

struct Config {
    LoggingConfig logging;
    AppConfig app;
    PathConfig paths;
    CatalogConfig catalogs;
    SceneConfig scene;
    AudioConfig audio;
    DisplayConfig display;
    AutoChangeConfig autoChange;
    VisualConfig visual;
    MessagesConfig messages;
};

struct ConfigBuildResult {
    Config config;
    std::vector<ConfigDiagnostic> diagnostics;

    bool ok() const;
};

class ConfigSchema {
public:
    Config build(const ConfigPatch& patch, DeferredLogBuffer& diagnostics) const;
};

class ConfigAcquisitionStrategy {
public:
    virtual ~ConfigAcquisitionStrategy();
    virtual ConfigPatch acquire(DeferredLogBuffer& diagnostics) const = 0;
};

class DefaultsConfigSource : public ConfigAcquisitionStrategy {
    ConfigPatch defaultsValue;

public:
    DefaultsConfigSource();
    explicit DefaultsConfigSource(const ConfigPatch& defaults);
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class IniTextConfigSource : public ConfigAcquisitionStrategy {
    std::string sourceNameValue;
    std::string textValue;

public:
    IniTextConfigSource(const std::string& sourceName, const std::string& text);
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class IniFileConfigSource : public ConfigAcquisitionStrategy {
    std::string pathValue;
    bool optionalValue;

public:
    IniFileConfigSource(const std::string& path, bool optional);
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class EnvironmentConfigSource : public ConfigAcquisitionStrategy {
    std::map<std::string, std::string> environmentValue;

public:
    explicit EnvironmentConfigSource(
        const std::map<std::string, std::string>& environment);
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class CommandLineConfigSource : public ConfigAcquisitionStrategy {
    std::vector<std::string> argumentsValue;

public:
    explicit CommandLineConfigSource(const std::vector<std::string>& arguments);
    explicit CommandLineConfigSource(std::vector<std::string>&& arguments);
    CommandLineConfigSource(int argc, char* argv[]);
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class ConfigurationBuilder {
    ConfigPatch patchValue;
    DeferredLogBuffer diagnosticsValue;
    ConfigSchema schemaValue;

public:
    ConfigurationBuilder();
    explicit ConfigurationBuilder(const DeferredLogBuffer& diagnostics);

    ConfigurationBuilder& addSource(const ConfigAcquisitionStrategy& source);
    ConfigurationBuilder& addDefaults(const ConfigPatch& defaults);
    ConfigurationBuilder& addDefaults();
    ConfigurationBuilder& addIniText(const std::string& sourceName,
        const std::string& text);
    ConfigurationBuilder& addIniFile(const std::string& path, bool optional);
    ConfigurationBuilder& addEnvironment(
        const std::map<std::string, std::string>& environment);
    ConfigurationBuilder& addEnvironmentVariables(
        const std::vector<std::string>& names);
    ConfigurationBuilder& addCommandLine(const std::vector<std::string>& args);
    ConfigurationBuilder& addCommandLine(std::vector<std::string>&& args);
    ConfigurationBuilder& addCommandLine(int argc, char* argv[]);

    const ConfigPatch& patch() const;
    ConfigBuildResult build() const;
};

ConfigPatch hardcodedDefaultConfigPatch();
std::vector<std::string> configArgumentsFromArgv(int argc, char* argv[]);
ConfigBuildResult buildStartupConfig(int argc, char* argv[]);

#endif
