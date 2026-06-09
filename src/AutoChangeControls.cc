/** @file
 * Option adapters for automatic scene-change settings.
 */

#include "AutoChangeControls.h"

#include "AutoChangeSettings.h"
#include "imath.h"
#include "ProcessServices.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static int nonNegative(int value) {
    return value < 0 ? 0 : value;
}

static int parseTimeText(const char* text, const char* name, int fallback,
    LogSink& log) {
    if (text == 0)
        return fallback;

    if (strstr(text, "sec") != NULL) {
        double seconds;
        if (sscanf(text, "%lfsec", &seconds) == 0) {
            log.error("Not a time value `%s' for option `%s'.\n", text, name);
            return fallback;
        }

        return nonNegative(int(seconds * 1000.0 + 0.5));
    }

    char* pos;
    int value = strtol(text, &pos, 0);
    if (text == pos) {
        log.error("Not a time value `%s' for option `%s'.\n", text, name);
        return fallback;
    }

    return nonNegative(value);
}

static int parseOnOffText(const char* text, const char* name, int fallback,
    LogSink& log) {
    if (text == 0)
        return fallback;

    if (!strncasecmp("yes", text, 3)
        || !strncasecmp("on", text, 2)
        || !strncasecmp("1", text, 1))
        return 1;

    if (!strncasecmp("no", text, 2)
        || !strncasecmp("off", text, 3)
        || !strncasecmp("0", text, 1))
        return 0;

    log.error("Illegal yes/no-value `%s' for option `%s'.\n", text, name);
    return fallback;
}

AutoChangeOptionAdapter::AutoChangeOptionAdapter(const char* name,
    AutoChangeSettings& settings, LogSink& log, AutoChangeControlField field,
    int time, int onOff)
    : Option(name)
    , settingsValue(settings)
    , logValue(log)
    , fieldValue(field)
    , timeValue(time)
    , onOffValue(onOff) { }

AutoChangeControlField AutoChangeOptionAdapter::field() const {
    return fieldValue;
}

int AutoChangeOptionAdapter::currentValue() const {
    switch (fieldValue) {
    case AutoChangeControlQuietMs:
        return settingsValue.quietMs();
    case AutoChangeControlWaitMinMs:
        return settingsValue.waitMinMs();
    case AutoChangeControlWaitRandomMs:
        return settingsValue.waitRandomMs();
    case AutoChangeControlCumulativeFireLevel:
        return settingsValue.cumulativeFireLevel();
    case AutoChangeControlLocked:
        return settingsValue.locked();
    case AutoChangeControlChangeLittle:
        return settingsValue.changeLittle();
    }

    return 0;
}

void AutoChangeOptionAdapter::setCurrentValue(int value) {
    if (onOffValue)
        value = value ? 1 : 0;
    else
        value = nonNegative(value);

    switch (fieldValue) {
    case AutoChangeControlQuietMs:
        settingsValue.setQuietMs(value);
        break;
    case AutoChangeControlWaitMinMs:
        settingsValue.setWaitMinMs(value);
        break;
    case AutoChangeControlWaitRandomMs:
        settingsValue.setWaitRandomMs(value);
        break;
    case AutoChangeControlCumulativeFireLevel:
        settingsValue.setCumulativeFireLevel(value);
        break;
    case AutoChangeControlLocked:
        settingsValue.setLocked(value);
        break;
    case AutoChangeControlChangeLittle:
        settingsValue.setChangeLittle(value);
        break;
    }
}

void AutoChangeOptionAdapter::change(int by) {
    if (onOffValue) {
        setCurrentValue(mod(currentValue() + by, 2));
    } else {
        setCurrentValue(currentValue() + by);
    }
}

void AutoChangeOptionAdapter::change(const char* to) {
    if (onOffValue) {
        setCurrentValue(parseOnOffText(to, name(), currentValue(), logValue));
    } else if (timeValue) {
        setCurrentValue(parseTimeText(to, name(), currentValue(), logValue));
    } else {
        setCurrentValue(to != 0 ? atoi(to) : currentValue());
    }
}

void AutoChangeOptionAdapter::setValue(int value) {
    setCurrentValue(value);
}

const char* AutoChangeOptionAdapter::text() const {
    static char textValue[64];

    if (onOffValue) {
        snprintf(textValue, sizeof(textValue), "%s",
            currentValue() ? " on" : "off");
    } else if (timeValue) {
        snprintf(textValue, sizeof(textValue), "%5.2f sec",
            double(currentValue()) / 1000.0);
    } else {
        snprintf(textValue, sizeof(textValue), "%7d", currentValue());
    }

    return textValue;
}

AutoChangeOptionAdapter::operator int() const {
    return currentValue();
}

AutoChangeControls::AutoChangeControls(AutoChangeSettings& settings,
    LogSink& log)
    : settingsValue(settings)
    , logValue(log)
    , quietOptionValue("quiet-change", settings, log, AutoChangeControlQuietMs,
          1, 0)
    , waitMinOptionValue("min-time", settings, log,
          AutoChangeControlWaitMinMs, 1, 0)
    , waitRandomOptionValue("random-time", settings,
          log, AutoChangeControlWaitRandomMs, 1, 0)
    , cumulativeFireLevelOptionValue("cumulative-fire-level", settings,
          log, AutoChangeControlCumulativeFireLevel, 0, 0)
    , lockedOptionValue("lock", settings, log, AutoChangeControlLocked, 0, 1)
    , changeLittleOptionValue("little", settings,
          log, AutoChangeControlChangeLittle, 0, 1) { }

AutoChangeSettings& AutoChangeControls::settings() {
    return settingsValue;
}

const AutoChangeSettings& AutoChangeControls::settings() const {
    return settingsValue;
}

Option& AutoChangeControls::quietOption() {
    return quietOptionValue;
}

Option& AutoChangeControls::waitMinOption() {
    return waitMinOptionValue;
}

Option& AutoChangeControls::waitRandomOption() {
    return waitRandomOptionValue;
}

Option& AutoChangeControls::cumulativeFireLevelOption() {
    return cumulativeFireLevelOptionValue;
}

Option& AutoChangeControls::lockedOption() {
    return lockedOptionValue;
}

Option& AutoChangeControls::changeLittleOption() {
    return changeLittleOptionValue;
}

Option& AutoChangeControls::option(AutoChangeControlField field) {
    switch (field) {
    case AutoChangeControlQuietMs:
        return quietOptionValue;
    case AutoChangeControlWaitMinMs:
        return waitMinOptionValue;
    case AutoChangeControlWaitRandomMs:
        return waitRandomOptionValue;
    case AutoChangeControlCumulativeFireLevel:
        return cumulativeFireLevelOptionValue;
    case AutoChangeControlLocked:
        return lockedOptionValue;
    case AutoChangeControlChangeLittle:
        return changeLittleOptionValue;
    }

    return quietOptionValue;
}

int AutoChangeControls::owns(const Option& option) const {
    return (&option == &quietOptionValue)
        || (&option == &waitMinOptionValue)
        || (&option == &waitRandomOptionValue)
        || (&option == &cumulativeFireLevelOptionValue)
        || (&option == &lockedOptionValue)
        || (&option == &changeLittleOptionValue);
}

void AutoChangeControls::toggleLock() {
    settingsValue.setLocked(!settingsValue.locked());
}

int AutoChangeControls::changeOptionBy(Option& option, int by) {
    if (!owns(option))
        return 0;

    option.change(by);
    return 1;
}

int AutoChangeControls::changeOptionTo(Option& option, const char* to) {
    if (!owns(option))
        return 0;

    option.change(to);
    return 1;
}
