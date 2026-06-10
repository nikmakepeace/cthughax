/** @file
 * Typed configuration model, acquisition sources, diagnostics, and builder.
 */

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

/** Deferred configuration diagnostic emitted before logging is configured. */
struct ConfigDiagnostic {
    ConfigDiagnosticSeverity severity;
    std::string source;
    std::string key;
    std::string message;

    /** Creates an empty informational diagnostic. */
    ConfigDiagnostic();

    /**
     * Creates a diagnostic with source/key context.
     *
     * @param severityValue Diagnostic severity.
     * @param sourceValue Human-readable source, such as a file path.
     * @param keyValue Configuration key or option associated with the message.
     * @param messageValue Human-readable diagnostic text.
     */
    ConfigDiagnostic(ConfigDiagnosticSeverity severityValue,
        const std::string& sourceValue, const std::string& keyValue,
        const std::string& messageValue);
};

/** Collects configuration diagnostics until logging is ready. */
class DeferredLogBuffer {
    std::vector<ConfigDiagnostic> diagnosticsValue;

public:
    /**
     * Appends a diagnostic.
     *
     * @param severity Diagnostic severity.
     * @param source Diagnostic source, such as a file path.
     * @param key Configuration key or option associated with the message.
     * @param message Human-readable diagnostic text.
     */
    void add(ConfigDiagnosticSeverity severity, const std::string& source,
        const std::string& key, const std::string& message);

    /**
     * Appends an informational diagnostic.
     *
     * @param source Diagnostic source, such as a file path.
     * @param key Configuration key or option associated with the message.
     * @param message Human-readable diagnostic text.
     */
    void info(const std::string& source, const std::string& key,
        const std::string& message);

    /**
     * Appends a warning diagnostic.
     *
     * @param source Diagnostic source, such as a file path.
     * @param key Configuration key or option associated with the message.
     * @param message Human-readable diagnostic text.
     */
    void warning(const std::string& source, const std::string& key,
        const std::string& message);

    /**
     * Appends an error diagnostic.
     *
     * @param source Diagnostic source, such as a file path.
     * @param key Configuration key or option associated with the message.
     * @param message Human-readable diagnostic text.
     */
    void error(const std::string& source, const std::string& key,
        const std::string& message);

    /**
     * Returns all diagnostics in insertion order.
     *
     * @return Buffered diagnostics.
     */
    const std::vector<ConfigDiagnostic>& diagnostics() const;

    /**
     * Reports whether any buffered diagnostic is an error.
     *
     * @return True if at least one error diagnostic is present.
     */
    bool hasErrors() const;

    /**
     * Appends all diagnostics from another buffer.
     *
     * @param other Buffer whose diagnostics should be appended.
     */
    void append(const DeferredLogBuffer& other);
};

/** Raw configuration key/value entry with provenance. */
struct ConfigEntry {
    std::string key;
    std::string value;
    std::string source;

    /** Creates an empty entry. */
    ConfigEntry();

    /**
     * Creates a key/value entry with source provenance.
     *
     * @param keyValue Canonical configuration key.
     * @param valueValue Text value associated with the key.
     * @param sourceValue Source that provided the entry.
     */
    ConfigEntry(const std::string& keyValue, const std::string& valueValue,
        const std::string& sourceValue);
};

/** Untyped configuration changes accumulated from acquisition sources. */
class ConfigPatch {
    std::map<std::string, ConfigEntry> entriesValue;
    std::map<std::string, std::vector<ConfigEntry> > repeatedEntriesValue;

public:
    /**
     * Sets a single-valued key, replacing any previous value.
     *
     * @param key Canonical configuration key.
     * @param value Text value to store.
     * @param source Source that provided the value.
     */
    void set(const std::string& key, const std::string& value,
        const std::string& source);

    /**
     * Appends a repeated key entry.
     *
     * @param key Canonical repeated configuration key.
     * @param value Text value to append.
     * @param source Source that provided the value.
     */
    void append(const std::string& key, const std::string& value,
        const std::string& source);

    /**
     * Reports whether a single-valued key exists.
     *
     * @param key Canonical configuration key.
     * @return True when a value was set for the key.
     */
    bool has(const std::string& key) const;

    /**
     * Returns a single-valued entry by key.
     *
     * @param key Canonical configuration key.
     * @return Entry pointer, or NULL when unset.
     */
    const ConfigEntry* entry(const std::string& key) const;

    /**
     * Returns a single-valued text value by key.
     *
     * @param key Canonical configuration key.
     * @return Value pointer, or NULL when unset.
     */
    const std::string* value(const std::string& key) const;

    /**
     * Returns all single-valued entries.
     *
     * @return Map of canonical keys to entries.
     */
    const std::map<std::string, ConfigEntry>& entries() const;

    /**
     * Returns repeated entries for a key.
     *
     * @param key Canonical repeated configuration key.
     * @return Entry vector pointer, or NULL when none exist.
     */
    const std::vector<ConfigEntry>* entriesFor(const std::string& key) const;

    /**
     * Merges another patch into this patch.
     *
     * @param patch Patch whose values should be applied after this one.
     */
    void mergeFrom(const ConfigPatch& patch);
};

struct LoggingConfig {
    int verbosity;

    /** Creates logging configuration with default values. */
    LoggingConfig();
};

struct AppConfig {
    int optionsSaveEnabled;

    /** Creates application configuration with default values. */
    AppConfig();
};

struct InputConfig {
    int escapeKeyEnabled;
    std::string keymapFile;

    /** Creates input configuration with default values. */
    InputConfig();
};

struct PathConfig {
    std::string extraLibraryPath;
    std::string iniFileOverride;
    std::vector<std::string> iniFiles;
    std::string continuationIniFile;

    /** Creates path configuration with default values. */
    PathConfig();
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
    std::string audioProcessing;

    /** Creates scene selection configuration with empty startup selections. */
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
    AudioOutputDriverId outputDriver;
    int mixerVolume;
    int pulseLatencyMs;
    std::string pulseServer;
    std::string outputDumpPath;
    std::string miniAudioPlaybackDeviceName;
    std::string miniAudioCaptureDeviceName;
    std::string dspDevicePath;
    std::string mixerDevicePath;
    std::vector<AudioMixerInitialVolumeConfig> mixerInitialVolumes;
    int nullOutputTargetLatencyMs;
    int pulseOutputTargetLatencyMs;
    int miniAudioOutputTargetLatencyMs;
    int dspOutputTargetLatencyMs;

    /** Creates audio configuration with default values. */
    AudioConfig();
};

/** Startup audio-analysis configuration. */
struct AudioAnalysisConfig {
    /** Noise floor used to decide whether a frame is noisy. */
    int minNoise;
    /** Fire detection sensitivity; lower values suppress smaller bursts. */
    int fireSensitivity;
    /** Fire detection metric source. */
    std::string fireSource;

    /** Creates audio-analysis configuration with default values. */
    AudioAnalysisConfig();
};

enum DisplayDriverId {
    DisplayDriverAuto,
    DisplayDriverX11,
    DisplayDriverSDL3
};

/** @return Stable configuration text for a display driver id. */
inline const char* displayDriverIdName(DisplayDriverId driver) {
    switch (driver) {
    case DisplayDriverAuto:
        return "auto";
    case DisplayDriverX11:
        return "x11";
    case DisplayDriverSDL3:
        return "sdl3";
    }

    return "unknown";
}

struct DisplayConfig {
    DisplayDriverId driver;
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

    /** Creates display configuration with default values. */
    DisplayConfig();
};

#ifdef CTH_XWIN
struct X11Config {
    int overrideRedirect;
    int privateCmap;
    int mitShm;
    int rootWindow;
    int fullscreen;
    int windowPositionEnabled;
    int windowPositionX;
    int windowPositionY;
    int panelEnabled;
    std::string fontName;
    std::string frameDumpDirectory;
    int frameDumpLimit;
    int frameDumpEvery;

    /** Creates X11 configuration with default values. */
    X11Config();
};
#endif

struct SDL3Config {
    int highPixelDensityEnabled;
    int resizableWindowEnabled;
    std::string rendererName;
    std::string frameDumpDirectory;
    int frameDumpLimit;
    int frameDumpEvery;

    /** Creates SDL3 display-driver configuration with default values. */
    SDL3Config();
};

struct AutoChangeConfig {
    int quietMs;
    int waitMinMs;
    int waitRandomMs;
    int waitRandomMinimumMs;
    int cumulativeFireLevel;
    int locked;
    int changeLittle;

    /** Creates automatic scene-change configuration with default values. */
    AutoChangeConfig();
};

struct EffectChoicePolicy {
    std::string catalogEntryKey;
    int enabled;

    /** Creates an empty effect-choice policy. */
    EffectChoicePolicy();

    /**
     * Creates an effect-choice policy for one catalog entry.
     *
     * @param catalogEntryKeyValue Canonical catalog entry key.
     * @param enabledValue Non-zero when the choice may be used.
     */
    EffectChoicePolicy(const std::string& catalogEntryKeyValue,
        int enabledValue);
};

struct EffectPresetPolicy {
    int slot;
    std::string catalogName;
    std::string choiceText;

    /** Creates an empty effect preset policy. */
    EffectPresetPolicy();

    /**
     * Creates an effect preset policy for one slot/catalog pair.
     *
     * @param slotValue Preset slot number.
     * @param catalogNameValue Catalog name stored in the preset.
     * @param choiceTextValue Choice text stored in the preset.
     */
    EffectPresetPolicy(int slotValue, const std::string& catalogNameValue,
        const std::string& choiceTextValue);
};

struct EffectPolicy {
    int imageFilesEnabled;
    std::string paletteSetFilterText;
    int useTranslatesEnabled;
    int useObjectsEnabled;
    std::vector<EffectChoicePolicy> allowedChoices;
    std::vector<EffectPresetPolicy> presets;

    /** Creates effect policy with default values. */
    EffectPolicy();
};

struct SceneTransitionPolicy {
    double paletteSmoothingChance;
    int paletteSmoothSeconds;

    /** Creates scene transition policy with default values. */
    SceneTransitionPolicy();
};

struct SceneScriptConfig {
    std::string directory;
    std::string script;

    /** Creates disabled scene-script configuration. */
    SceneScriptConfig();
};

struct MessagesConfig {
    int quietMessageMs;
    int quietMessageDurationMs;
    std::string quietMessageFile;
    int qotdEnabled;
    int qotdPrefetchTimeoutMs;
    std::string qotdServer;
    std::string qotdPort;

    /** Creates message and QOTD configuration with default values. */
    MessagesConfig();
};

struct Config {
    LoggingConfig logging;
    AppConfig app;
    InputConfig input;
    PathConfig paths;
    SceneConfig scene;
    AudioConfig audio;
    AudioAnalysisConfig audioAnalysis;
    DisplayConfig display;
#ifdef CTH_XWIN
    X11Config x11;
#endif
    SDL3Config sdl3;
    AutoChangeConfig autoChange;
    EffectPolicy effectPolicy;
    SceneTransitionPolicy sceneTransition;
    SceneScriptConfig sceneScript;
    MessagesConfig messages;
};

struct ConfigBuildResult {
    Config config;
    std::vector<ConfigDiagnostic> diagnostics;
    int helpRequested;
    int versionRequested;

    /**
     * Reports whether acquisition and typed conversion succeeded.
     *
     * @return True when no error diagnostics were produced.
     */
    bool ok() const;
};

/** Converts raw configuration patches into typed Config values. */
class ConfigSchema {
public:
    /**
     * Builds typed configuration from a raw patch.
     *
     * @param patch Raw acquired configuration patch.
     * @param diagnostics Buffer that receives conversion diagnostics.
     * @return Typed configuration assembled from the patch.
     */
    Config build(const ConfigPatch& patch, DeferredLogBuffer& diagnostics) const;
};

/** Interface for one configuration acquisition source. */
class ConfigAcquisitionStrategy {
public:
    /** Destroys the acquisition strategy interface. */
    virtual ~ConfigAcquisitionStrategy();

    /**
     * Acquires a raw configuration patch.
     *
     * @param diagnostics Buffer that receives source diagnostics.
     * @return Raw patch acquired from this source.
     */
    virtual ConfigPatch acquire(DeferredLogBuffer& diagnostics) const = 0;
};

/** Acquisition source that contributes hardcoded defaults. */
class DefaultsConfigSource : public ConfigAcquisitionStrategy {
    ConfigPatch defaultsValue;

public:
    /** Creates a source using the built-in default patch. */
    DefaultsConfigSource();

    /**
     * Creates a source using an explicit defaults patch.
     *
     * @param defaults Patch to contribute when acquired.
     */
    explicit DefaultsConfigSource(const ConfigPatch& defaults);

    /**
     * Acquires the configured defaults patch.
     *
     * @param diagnostics Unused diagnostics buffer retained for interface parity.
     * @return Defaults patch.
     */
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class IniTextConfigSource : public ConfigAcquisitionStrategy {
    std::string sourceNameValue;
    std::string textValue;

public:
    /**
     * Creates an ini-text acquisition source.
     *
     * @param sourceName Name reported in diagnostics.
     * @param text Ini text to parse.
     */
    IniTextConfigSource(const std::string& sourceName, const std::string& text);

    /**
     * Parses the ini text into a raw patch.
     *
     * @param diagnostics Buffer that receives parse diagnostics.
     * @return Raw patch acquired from the ini text.
     */
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class IniFileConfigSource : public ConfigAcquisitionStrategy {
    std::string pathValue;
    bool optionalValue;

public:
    /**
     * Creates an ini-file acquisition source.
     *
     * @param path File path to read.
     * @param optional True when a missing file should not be an error.
     */
    IniFileConfigSource(const std::string& path, bool optional);

    /**
     * Reads and parses the ini file into a raw patch.
     *
     * @param diagnostics Buffer that receives file and parse diagnostics.
     * @return Raw patch acquired from the ini file.
     */
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class EnvironmentConfigSource : public ConfigAcquisitionStrategy {
    std::map<std::string, std::string> environmentValue;

public:
    /**
     * Creates an environment acquisition source from explicit name/value pairs.
     *
     * @param environment Environment map to inspect.
     */
    explicit EnvironmentConfigSource(
        const std::map<std::string, std::string>& environment);

    /**
     * Converts supported environment variables into a raw patch.
     *
     * @param diagnostics Buffer that receives conversion diagnostics.
     * @return Raw patch acquired from the environment map.
     */
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class CommandLineConfigSource : public ConfigAcquisitionStrategy {
    std::vector<std::string> argumentsValue;

public:
    /**
     * Creates a command-line source from an argument vector.
     *
     * @param arguments Program arguments including argv[0] when available.
     */
    explicit CommandLineConfigSource(const std::vector<std::string>& arguments);

    /**
     * Creates a command-line source by taking ownership of an argument vector.
     *
     * @param arguments Program arguments including argv[0] when available.
     */
    explicit CommandLineConfigSource(std::vector<std::string>&& arguments);

    /**
     * Creates a command-line source from argc/argv.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     */
    CommandLineConfigSource(int argc, char* argv[]);

    /**
     * Converts supported command-line options into a raw patch.
     *
     * @param diagnostics Buffer that receives parse diagnostics.
     * @return Raw patch acquired from the command line.
     */
    ConfigPatch acquire(DeferredLogBuffer& diagnostics) const;
};

class ConfigurationBuilder {
    ConfigPatch patchValue;
    DeferredLogBuffer diagnosticsValue;
    ConfigSchema schemaValue;
    int helpRequestedValue;
    int versionRequestedValue;

public:
    /** Creates an empty builder with no diagnostics. */
    ConfigurationBuilder();

    /**
     * Creates a builder with existing diagnostics.
     *
     * @param diagnostics Diagnostics to carry into the build result.
     */
    explicit ConfigurationBuilder(const DeferredLogBuffer& diagnostics);

    /**
     * Adds one acquisition source to the builder.
     *
     * @param source Source to acquire and merge.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addSource(const ConfigAcquisitionStrategy& source);

    /**
     * Adds an explicit defaults patch.
     *
     * @param defaults Defaults patch to merge.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addDefaults(const ConfigPatch& defaults);

    /**
     * Adds the built-in defaults patch.
     *
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addDefaults();

    /**
     * Adds configuration parsed from ini text.
     *
     * @param sourceName Name reported in diagnostics.
     * @param text Ini text to parse.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addIniText(const std::string& sourceName,
        const std::string& text);

    /**
     * Adds configuration parsed from an ini file.
     *
     * @param path File path to read.
     * @param optional True when a missing file should not be an error.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addIniFile(const std::string& path, bool optional);

    /**
     * Adds configuration from explicit environment variables.
     *
     * @param environment Environment name/value map.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addEnvironment(
        const std::map<std::string, std::string>& environment);

    /**
     * Adds configuration from selected process environment variables.
     *
     * @param names Environment variable names to read.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addEnvironmentVariables(
        const std::vector<std::string>& names);

    /**
     * Adds configuration parsed from an argument vector.
     *
     * @param args Program arguments including argv[0] when available.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addCommandLine(const std::vector<std::string>& args);

    /**
     * Adds configuration parsed from a moved argument vector.
     *
     * @param args Program arguments including argv[0] when available.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addCommandLine(std::vector<std::string>&& args);

    /**
     * Adds configuration parsed from argc/argv.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     * @return This builder for chaining.
     */
    ConfigurationBuilder& addCommandLine(int argc, char* argv[]);

    /**
     * Returns the merged raw patch accumulated so far.
     *
     * @return Current raw patch.
     */
    const ConfigPatch& patch() const;

    /**
     * Builds the final typed configuration result.
     *
     * @return Typed config, diagnostics, and help-request status.
     */
    ConfigBuildResult build() const;
};

/**
 * Builds the process-wide hardcoded default patch.
 *
 * @return Raw default configuration patch.
 */
ConfigPatch hardcodedDefaultConfigPatch();

/**
 * Copies argc/argv into an STL argument vector.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Argument strings; empty when argc is zero or argv is NULL.
 */
std::vector<std::string> configArgumentsFromArgv(int argc, char* argv[]);

/**
 * Performs full startup acquisition in production precedence order.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Startup configuration, diagnostics, and help-request status.
 */
ConfigBuildResult buildStartupConfig(int argc, char* argv[]);

#endif
