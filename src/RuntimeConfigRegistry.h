/** @file
 * Runtime configuration registry used by persistence.
 */

#ifndef CTHUGHA_RUNTIME_CONFIG_REGISTRY_H
#define CTHUGHA_RUNTIME_CONFIG_REGISTRY_H

#include "Configuration.h"

#include <vector>

class SceneCommands;

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
 * Legacy-backed contributor for the runtime values still stored in globals.
 */
class LegacyRuntimeConfigContributor : public RuntimeConfigContributor {
    SceneCommands& sceneCommands;

public:
    /**
     * Creates a contributor backed by SceneCommands plus transitional globals.
     *
     * @param sceneCommands_ Scene command facade used to read live scene state.
     */
    explicit LegacyRuntimeConfigContributor(SceneCommands& sceneCommands_);

    /**
     * Overlays the current scene, display, auto-change, and policy values.
     *
     * @param config Snapshot being prepared for persistence.
     */
    virtual void contribute(Config& config) const;
};

#endif
