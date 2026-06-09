/** @file
 * Option adapters for automatic scene-change settings.
 */

#ifndef CTHUGHA_AUTO_CHANGE_CONTROLS_H
#define CTHUGHA_AUTO_CHANGE_CONTROLS_H

#include "Option.h"

class AutoChangeSettings;
class LogSink;

enum AutoChangeControlField {
    AutoChangeControlQuietMs,
    AutoChangeControlWaitMinMs,
    AutoChangeControlWaitRandomMs,
    AutoChangeControlCumulativeFireLevel,
    AutoChangeControlLocked,
    AutoChangeControlChangeLittle
};

/**
 * Option adapter for one AutoChangeSettings field.
 */
class AutoChangeOptionAdapter : public Option {
    AutoChangeSettings& settingsValue;
    LogSink& logValue;
    AutoChangeControlField fieldValue;
    int timeValue;
    int onOffValue;

    /** @return Current value from the backed settings field. */
    int currentValue() const;

    /** Writes a normalized value to the backed settings field. */
    void setCurrentValue(int value);

public:
    /**
     * Creates an Option view of one automatic scene-change setting.
     *
     * @param name Option name retained for command/status diagnostics.
     * @param settings Backing settings object; must outlive this adapter.
     * @param log Diagnostic sink.
     * @param field Backed setting field.
     * @param time Nonzero when text should be formatted and parsed as time.
     * @param onOff Nonzero when value should be treated as a boolean toggle.
     */
    AutoChangeOptionAdapter(const char* name, AutoChangeSettings& settings,
        LogSink& log, AutoChangeControlField field, int time, int onOff);

    /** @return Setting field exposed by this adapter. */
    AutoChangeControlField field() const;

    /** Applies a relative value change to the backed setting. */
    virtual void change(int by);

    /** Applies a text value change to the backed setting. */
    virtual void change(const char* to);

    /** Sets the backed setting without relative stepping. */
    virtual void setValue(int value);

    /** @return Text representation of the backed setting. */
    virtual const char* text() const;

    /** @return Current integer value from the backed setting. */
    virtual operator int() const;
};

/**
 * Owns the Option adapters used to mutate AutoChangeSettings at runtime.
 */
class AutoChangeControls {
    AutoChangeSettings& settingsValue;
    LogSink& logValue;
    AutoChangeOptionAdapter quietOptionValue;
    AutoChangeOptionAdapter waitMinOptionValue;
    AutoChangeOptionAdapter waitRandomOptionValue;
    AutoChangeOptionAdapter cumulativeFireLevelOptionValue;
    AutoChangeOptionAdapter lockedOptionValue;
    AutoChangeOptionAdapter changeLittleOptionValue;

public:
    /**
     * Creates automatic scene-change Option adapters.
     *
     * @param settings Backing settings object; must outlive these controls.
     * @param log Diagnostic sink.
     */
    AutoChangeControls(AutoChangeSettings& settings, LogSink& log);

    /** @return Backing settings object. */
    AutoChangeSettings& settings();

    /** @return Backing settings object. */
    const AutoChangeSettings& settings() const;

    /** @return Option adapter for quiet-change milliseconds. */
    Option& quietOption();

    /** @return Option adapter for minimum wait milliseconds. */
    Option& waitMinOption();

    /** @return Option adapter for random wait milliseconds. */
    Option& waitRandomOption();

    /** @return Option adapter for cumulative fire threshold. */
    Option& cumulativeFireLevelOption();

    /** @return Option adapter for auto-change lock. */
    Option& lockedOption();

    /** @return Option adapter for little-changes-only mode. */
    Option& changeLittleOption();

    /**
     * Returns an Option adapter for a field.
     *
     * @param field Field to expose.
     * @return Option adapter for the field.
     */
    Option& option(AutoChangeControlField field);

    /**
     * Reports whether an option is one of these adapters.
     *
     * @param option Option to inspect.
     * @return Nonzero when the option belongs to these controls.
     */
    int owns(const Option& option) const;

    /** Toggles the auto-change lock setting. */
    void toggleLock();

    /**
     * Attempts to apply a relative change through one of these adapters.
     *
     * @param option Option to inspect and maybe change.
     * @param by Relative change to apply.
     * @return Nonzero when the option was handled.
     */
    int changeOptionBy(Option& option, int by);

    /**
     * Attempts to apply a text change through one of these adapters.
     *
     * @param option Option to inspect and maybe change.
     * @param to Text value to apply.
     * @return Nonzero when the option was handled.
     */
    int changeOptionTo(Option& option, const char* to);
};

#endif
