/** @file
 * Owned automatic scene-change settings.
 */

#ifndef CTHUGHA_AUTO_CHANGE_SETTINGS_H
#define CTHUGHA_AUTO_CHANGE_SETTINGS_H

#include "Configuration.h"

/**
 * Read/write settings port for automatic scene-change policy.
 *
 * SceneChangeScheduler reads this object, runtime controls mutate it, and
 * persistence snapshots it. Implementations own the values; callers must not
 * depend on legacy global Option storage for these fields.
 */
class AutoChangeSettings {
public:
    /** Releases the settings interface. */
    virtual ~AutoChangeSettings();

    /** @return Quiet interval before an interrupted-silence change, in ms. */
    virtual int quietMs() const = 0;

    /** @param value Quiet interval before an interrupted-silence change, in ms. */
    virtual void setQuietMs(int value) = 0;

    /** @return Minimum time between timed automatic changes, in ms. */
    virtual int waitMinMs() const = 0;

    /** @param value Minimum time between timed automatic changes, in ms. */
    virtual void setWaitMinMs(int value) = 0;

    /** @return Additional random wait window selected by the user, in ms. */
    virtual int waitRandomMs() const = 0;

    /** @param value Additional random wait window selected by the user, in ms. */
    virtual void setWaitRandomMs(int value) = 0;

    /**
     * @return Minimum random wait range used internally to avoid rand() % 0.
     */
    virtual int waitRandomMinimumMs() const = 0;

    /**
     * @return Random wait range safe for modulo arithmetic.
     */
    virtual int waitRandomRangeMs() const = 0;

    /** @return Cumulative fire threshold for automatic changes. */
    virtual int cumulativeFireLevel() const = 0;

    /** @param value Cumulative fire threshold for automatic changes. */
    virtual void setCumulativeFireLevel(int value) = 0;

    /** @return Nonzero when automatic scene changes are locked. */
    virtual int locked() const = 0;

    /** @param value Nonzero to lock automatic scene changes. */
    virtual void setLocked(int value) = 0;

    /** @return Nonzero when SceneChangeScheduler should change one option only. */
    virtual int changeLittle() const = 0;

    /** @param value Nonzero to change one option only. */
    virtual void setChangeLittle(int value) = 0;

    /** @return Current settings as an ini-persistable config snapshot. */
    virtual AutoChangeConfig config() const = 0;
};

/**
 * Concrete Application-owned automatic scene-change settings.
 */
class OwnedAutoChangeSettings : public AutoChangeSettings {
    AutoChangeConfig configValue;

    /** Clamps a numeric setting to the non-negative range used by options. */
    static int nonNegative(int value);

    /** Normalizes all mutable fields after construction or whole-config apply. */
    void normalize();

public:
    /**
     * Creates settings from startup configuration.
     *
     * @param config Startup automatic scene-change configuration.
     */
    explicit OwnedAutoChangeSettings(
        const AutoChangeConfig& config = AutoChangeConfig());

    /**
     * Replaces all settings from a configuration snapshot.
     *
     * @param config Automatic scene-change configuration.
     */
    void applyConfig(const AutoChangeConfig& config);

    /** @return Quiet interval before an interrupted-silence change, in ms. */
    virtual int quietMs() const;

    /** @param value Quiet interval before an interrupted-silence change, in ms. */
    virtual void setQuietMs(int value);

    /** @return Minimum time between timed automatic changes, in ms. */
    virtual int waitMinMs() const;

    /** @param value Minimum time between timed automatic changes, in ms. */
    virtual void setWaitMinMs(int value);

    /** @return Additional random wait window selected by the user, in ms. */
    virtual int waitRandomMs() const;

    /** @param value Additional random wait window selected by the user, in ms. */
    virtual void setWaitRandomMs(int value);

    /** @return Minimum random wait range used internally. */
    virtual int waitRandomMinimumMs() const;

    /** @return Random wait range safe for modulo arithmetic. */
    virtual int waitRandomRangeMs() const;

    /** @return Cumulative fire threshold for automatic changes. */
    virtual int cumulativeFireLevel() const;

    /** @param value Cumulative fire threshold for automatic changes. */
    virtual void setCumulativeFireLevel(int value);

    /** @return Nonzero when automatic scene changes are locked. */
    virtual int locked() const;

    /** @param value Nonzero to lock automatic scene changes. */
    virtual void setLocked(int value);

    /** @return Nonzero when SceneChangeScheduler should change one option only. */
    virtual int changeLittle() const;

    /** @param value Nonzero to change one option only. */
    virtual void setChangeLittle(int value);

    /** @return Current settings as an ini-persistable config snapshot. */
    virtual AutoChangeConfig config() const;
};

#endif
