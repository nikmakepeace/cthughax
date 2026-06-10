/** @file
 * Runtime configuration registry used by persistence.
 */

#ifndef CTHUGHA_RUNTIME_CONFIG_REGISTRY_H
#define CTHUGHA_RUNTIME_CONFIG_REGISTRY_H

#include "Configuration.h"

#include <vector>

class AutoChangeSettings;
class AcousticContext;
class AudioProcessingState;
class DisplayPresentationSettings;
class Option;

/**
 * Adds one runtime owner's current state to a Config snapshot.
 */
class RuntimeConfigContributor {
public:
    /** Destroys the contributor interface. */
    virtual ~RuntimeConfigContributor();

    /**
     * Overlays current runtime state onto a mutable Config snapshot.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const = 0;
};

/**
 * Builds current runtime Config snapshots from a startup baseline.
 */
class RuntimeConfigRegistry {
    Config baselineValue;
    std::vector<const RuntimeConfigContributor*> contributors;

public:
    /**
     * Creates a registry with the startup configuration as its baseline.
     *
     * @param baseline Startup Config used for values that are not runtime-owned.
     */
    explicit RuntimeConfigRegistry(const Config& baseline);

    /**
     * Replaces the startup baseline used for future currentConfig() calls.
     *
     * @param baseline New baseline Config.
     */
    void setBaseline(const Config& baseline);

    /**
     * Adds a contributor that can overlay live runtime state.
     *
     * @param contributor Contributor retained by reference; caller owns it.
     */
    void addContributor(const RuntimeConfigContributor& contributor);

    /**
     * Builds a Config snapshot suitable for persistence.
     *
     * @return Baseline Config after all contributors have overlaid live state.
     */
    Config currentConfig() const;
};

/**
 * Contributes Display-owned runtime presentation settings.
 */
class DisplayRuntimeConfigContributor : public RuntimeConfigContributor {
    const DisplayPresentationSettings& displaySettings;

public:
    /**
     * Creates a contributor over Display-owned presentation settings.
     *
     * @param displaySettings_ Settings owned by DisplaySystem.
     */
    explicit DisplayRuntimeConfigContributor(
        const DisplayPresentationSettings& displaySettings_);

    /**
     * Overlays Display-owned runtime presentation state.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const;
};

/**
 * Contributes Audio-owned runtime processing state.
 */
class AudioRuntimeConfigContributor : public RuntimeConfigContributor {
    const AudioProcessingState& audioProcessingState;

public:
    /**
     * Creates a contributor over Audio-owned processing state.
     *
     * @param audioProcessingState_ Runtime-owned audio processing state.
     */
    explicit AudioRuntimeConfigContributor(
        const AudioProcessingState& audioProcessingState_);

    /**
     * Overlays Audio-owned runtime processing state.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const;
};

/**
 * Contributes Application-owned runtime settings.
 */
class ApplicationRuntimeConfigContributor : public RuntimeConfigContributor {
    const AutoChangeSettings& autoChangeSettings;
    const AcousticContext& acousticContext;
    const Option& quietMessageOption;

public:
    /**
     * Creates a contributor over Application-owned runtime settings.
     *
     * @param autoChangeSettings_ Runtime-owned automatic scene-change settings.
     * @param acousticContext_ Runtime-owned acoustic analysis settings.
     * @param quietMessageOption_ Runtime quiet-message threshold option.
     */
    ApplicationRuntimeConfigContributor(
        const AutoChangeSettings& autoChangeSettings_,
        const AcousticContext& acousticContext_,
        const Option& quietMessageOption_);

    /**
     * Overlays Application-owned runtime settings.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const;
};

/** Transitional contributor for legacy values not yet split by subsystem. */
class LegacyRuntimeConfigContributor : public RuntimeConfigContributor {
public:
    /** Creates a contributor over the remaining legacy presentation globals. */
    LegacyRuntimeConfigContributor();

    /**
     * Overlays values still owned by legacy scene/effect-policy globals.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const;
};

#endif
