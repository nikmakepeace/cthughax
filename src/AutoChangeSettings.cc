/** @file
 * Owned automatic scene-change settings implementation.
 */

#include "AutoChangeSettings.h"

AutoChangeSettings::~AutoChangeSettings() { }

int OwnedAutoChangeSettings::nonNegative(int value) {
    return value < 0 ? 0 : value;
}

void OwnedAutoChangeSettings::normalize() {
    configValue.quietMs = nonNegative(configValue.quietMs);
    configValue.waitMinMs = nonNegative(configValue.waitMinMs);
    configValue.waitRandomMs = nonNegative(configValue.waitRandomMs);
    configValue.waitRandomMinimumMs = configValue.waitRandomMinimumMs > 0
        ? configValue.waitRandomMinimumMs
        : 1;
    configValue.cumulativeFireLevel
        = nonNegative(configValue.cumulativeFireLevel);
    configValue.locked = configValue.locked ? 1 : 0;
    configValue.changeLittle = configValue.changeLittle ? 1 : 0;
}

OwnedAutoChangeSettings::OwnedAutoChangeSettings(
    const AutoChangeConfig& config)
    : configValue(config) {
    normalize();
}

void OwnedAutoChangeSettings::applyConfig(const AutoChangeConfig& config) {
    configValue = config;
    normalize();
}

int OwnedAutoChangeSettings::quietMs() const {
    return configValue.quietMs;
}

void OwnedAutoChangeSettings::setQuietMs(int value) {
    configValue.quietMs = nonNegative(value);
}

int OwnedAutoChangeSettings::waitMinMs() const {
    return configValue.waitMinMs;
}

void OwnedAutoChangeSettings::setWaitMinMs(int value) {
    configValue.waitMinMs = nonNegative(value);
}

int OwnedAutoChangeSettings::waitRandomMs() const {
    return configValue.waitRandomMs;
}

void OwnedAutoChangeSettings::setWaitRandomMs(int value) {
    configValue.waitRandomMs = nonNegative(value);
}

int OwnedAutoChangeSettings::waitRandomMinimumMs() const {
    return configValue.waitRandomMinimumMs;
}

int OwnedAutoChangeSettings::waitRandomRangeMs() const {
    int range = configValue.waitRandomMs > configValue.waitRandomMinimumMs
        ? configValue.waitRandomMs
        : configValue.waitRandomMinimumMs;
    return range > 0 ? range : 1;
}

int OwnedAutoChangeSettings::cumulativeFireLevel() const {
    return configValue.cumulativeFireLevel;
}

void OwnedAutoChangeSettings::setCumulativeFireLevel(int value) {
    configValue.cumulativeFireLevel = nonNegative(value);
}

int OwnedAutoChangeSettings::locked() const {
    return configValue.locked;
}

void OwnedAutoChangeSettings::setLocked(int value) {
    configValue.locked = value ? 1 : 0;
}

int OwnedAutoChangeSettings::changeLittle() const {
    return configValue.changeLittle;
}

void OwnedAutoChangeSettings::setChangeLittle(int value) {
    configValue.changeLittle = value ? 1 : 0;
}

AutoChangeConfig OwnedAutoChangeSettings::config() const {
    return configValue;
}
