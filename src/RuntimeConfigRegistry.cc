/** @file
 * Core runtime configuration registry implementation.
 */

#include "RuntimeConfigRegistry.h"

RuntimeConfigContributor::~RuntimeConfigContributor() { }

RuntimeConfigRegistry::RuntimeConfigRegistry(const Config& baseline)
    : baselineValue(baseline)
    , contributors() { }

void RuntimeConfigRegistry::setBaseline(const Config& baseline) {
    baselineValue = baseline;
}

void RuntimeConfigRegistry::addContributor(
    const RuntimeConfigContributor& contributor) {
    contributors.push_back(&contributor);
}

Config RuntimeConfigRegistry::currentConfig() const {
    Config config = baselineValue;

    for (std::vector<const RuntimeConfigContributor*>::const_iterator it
             = contributors.begin();
         it != contributors.end(); ++it) {
        (*it)->contribute(config);
    }

    return config;
}
