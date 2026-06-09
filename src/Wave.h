#ifndef __WAVE_H
#define __WAVE_H

#include <vector>

class FrameRenderTarget;
class RandomSource;
class FrameGeneratorContext;
class LogSink;

/**
 * 3D wire object used by object-based wave renderers.
 *
 * A WObject is a line list. Each line contains two integer 3-space endpoints.
 * The list is terminated by a final line whose endpoint coordinates are all -1.
 */
typedef int WObject[2][3];

/**
 * Scene/display configuration passed to wave renderers.
 */
class WaveConfig {
public:
    /** Wave-scale option index. Interpreted by individual wave renderers. */
    int waveScale;

    /** Palette color-table option index used by tableColor(). */
    int table;

    /** Borrowed 3D object line list, or NULL for non-object waves. */
    WObject* object;

    /** Current buffer width in pixels. */
    int bufferWidth;

    /** Current buffer height in pixels. */
    int bufferHeight;

    WaveConfig();

    /**
     * @param waveScale_ Wave-scale option index.
     * @param table_ Palette color-table option index.
     * @param object_ Borrowed 3D object line list, or NULL.
     * @param bufferWidth_ Buffer width in pixels.
     * @param bufferHeight_ Buffer height in pixels.
     */
    WaveConfig(int waveScale_, int table_, WObject* object_,
        int bufferWidth_, int bufferHeight_);

    /**
     * @param other Configuration to compare with.
     * @return Nonzero when wave-scale, table, object, and dimensions match.
     */
    int sameAs(const WaveConfig& other) const;
};

/**
 * Type-erased persistent state owned by WaveFilter for the current Wave.
 *
 * Individual wave implementations request their own state struct with state<T>().
 * The state is cleared when the selected Wave changes or configuration requires it.
 */
class WaveState {
    void* value;
    void (*destroyValue)(void*);

    template <class T>
    static void destroy(void* value_) {
        delete static_cast<T*>(value_);
    }

public:
    WaveState()
        : value(0)
        , destroyValue(0) { }
    ~WaveState() {
        clear();
    }

    /** Deletes any currently stored wave-specific state. */
    void clear() {
        if (destroyValue != 0)
            (*destroyValue)(value);
        value = 0;
        destroyValue = 0;
    }

    /**
     * Gets or lazily creates state for a wave implementation.
     *
     * @tparam T Wave-specific state type.
     * @return Mutable state object owned by this WaveState.
     */
    template <class T>
    T& get() {
        if (value == 0) {
            value = new T;
            destroyValue = &WaveState::destroy<T>;
        }
        return *static_cast<T*>(value);
    }
};

/**
 * Small lookup-table cache shared by wave renderers.
 */
class WaveLookupTables {
    int sineWidth;
    std::vector<int> sineValues;
    std::vector<int> legacySineValues;
    std::vector<double> degreeSineValues;

public:
    WaveLookupTables();

    /**
     * Builds or returns a sine table sized to the buffer width.
     *
     * @param width Requested table width in pixels/samples.
     * @return Pointer to width signed values scaled around +/-128, or NULL.
     */
    const int* sineForWidth(int width);

    /**
     * Returns the historical 320-step sine table value.
     *
     * @param index Angle index in the legacy 0..319 circle.
     * @return Sine value scaled around +/-128.
     */
    int legacySine(int index);

    /**
     * Returns sine for a degree angle.
     *
     * @param degrees Angle in degrees.
     * @return Sine in floating-point units.
     */
    double sineDegrees(int degrees);

    /**
     * Returns cosine for a degree angle.
     *
     * @param degrees Angle in degrees.
     * @return Cosine in floating-point units.
     */
    double cosineDegrees(int degrees);
};

/**
 * Per-frame runtime facade passed to wave renderer functions.
 */
class WaveRuntime {
    int needsConfigurationValue;
    WaveState& stateValue;
    WaveLookupTables& lookupTables;
    RandomSource& randomSource;
    LogSink& logValue;
    int fireBudgetValue;

public:
    /** Wave-scale option index copied from WaveConfig. */
    int waveScale;

    /** Palette color-table option index copied from WaveConfig. */
    int table;

    /** Borrowed 3D object line list copied from WaveConfig, or NULL. */
    WObject* object;

    /**
     * @param config Scene/display configuration for this wave.
     * @param needsConfiguration_ Nonzero when renderer should rebuild config-dependent state.
     * @param state_ Persistent state storage owned by WaveFilter.
     * @param lookupTables_ Shared wave lookup-table cache.
     * @param randomSource_ Random source owned by the application lifecycle.
     * @param log Diagnostics sink for this frame render.
     * @param fireBudget Acoustic fire amount for this visual frame.
     */
    WaveRuntime(const WaveConfig& config, int needsConfiguration_, WaveState& state_,
        WaveLookupTables& lookupTables_, RandomSource& randomSource_,
        LogSink& log, int fireBudget);

    /** @return Nonzero when renderer should rebuild configuration-dependent state. */
    int needsConfiguration() const;

    /** @return Remaining acoustic fire budget for this frame. */
    int fire() const;

    /** Clears the current frame fire budget after a renderer consumes it. */
    void consumeFire();

    /**
     * Scales the current frame fire budget.
     *
     * @param numerator Multiplicative numerator.
     * @param denominator Multiplicative denominator; zero clears fire.
     */
    void scaleFire(int numerator, int denominator);

    /**
     * @param width Requested table width in pixels/samples.
     * @return Cached sine table, or NULL.
     */
    const int* sineForWidth(int width);

    /**
     * Returns the historical 320-step sine table value.
     *
     * @param index Angle index in the legacy 0..319 circle.
     * @return Sine value scaled around +/-128.
     */
    int legacySine(int index);

    /**
     * Returns sine for a degree angle from generator-owned tables.
     *
     * @param degrees Angle in degrees.
     * @return Sine in floating-point units.
     */
    double sineDegrees(int degrees) const;

    /**
     * Returns cosine for a degree angle from generator-owned tables.
     *
     * @param degrees Angle in degrees.
     * @return Cosine in floating-point units.
     */
    double cosineDegrees(int degrees) const;

    /**
     * Returns a random integer in [0, exclusiveMax).
     *
     * @param exclusiveMax Upper exclusive bound. Values <= 1 return 0.
     * @return Random integer in the requested range.
     */
    int randomInt(int exclusiveMax);

    /**
     * Returns a random integer in [-magnitude, magnitude].
     *
     * @param magnitude Maximum absolute value.
     * @return Random signed integer in the requested symmetric range.
     */
    int randomCenteredInt(int magnitude);

    /** @return Random floating-point value in [0.0, 1.0]. */
    double randomUnit();

    /** @return Diagnostics sink for this wave render. */
    LogSink& log() const;

    /**
     * Gets or lazily creates wave-specific persistent state.
     *
     * @tparam T State type for the current wave implementation.
     * @return Mutable state object.
     */
    template <class T>
    T& state() {
        return stateValue.get<T>();
    }
};

/**
 * Catalog entry and executor for one wave renderer.
 */
class Wave {
public:
    /**
     * Wave renderer function.
     *
     * @param buffer Active indexed pixel buffer to draw into.
     * @param context Current frame render audio/time context.
     * @param runtime Wave config, state, lookup tables, and fire budget.
     */
    typedef void (*Function)(FrameRenderTarget& buffer,
        const FrameGeneratorContext& context, WaveRuntime& runtime);

    /**
     * Predicate that decides whether a wave can run with a config.
     *
     * @param config Current wave configuration.
     * @return Nonzero when the wave can be selected/run.
     */
    typedef int (*CanRunFunction)(const WaveConfig& config);

private:
    Function functionValue;
    CanRunFunction canRunFunctionValue;
    const char* nameValue;
    const char* descriptionValue;

public:
    /**
     * Creates a wave catalog entry.
     *
     * @param function Renderer function, or NULL for a no-op entry.
     * @param name Short option/UI name.
     * @param description Human-readable description.
     * @param canRunFunction Optional config predicate; NULL means always runnable.
     */
    Wave(Function function, const char* name, const char* description,
        CanRunFunction canRunFunction = 0);

    /** @return Short option/UI name. */
    const char* name() const;

    /** @return Human-readable description. */
    const char* description() const;

    /**
     * @param config Current wave configuration.
     * @return Nonzero when this wave can run with the config.
     */
    int canRun(const WaveConfig& config) const;

    /**
     * Executes the wave renderer for one visual frame.
     *
     * @param buffer Active indexed pixel buffer to draw into.
     * @param context Current frame render audio/time context.
     * @param config Wave-scale/table/object/dimension settings.
     * @param needsConfiguration Nonzero when renderer should rebuild state.
     * @param state Persistent state storage for this wave.
     * @param lookupTables Shared lookup-table cache.
     * @param randomSource Random source used by renderer effects.
     * @param log Diagnostics sink for this frame render.
     */
    void execute(FrameRenderTarget& buffer, const FrameGeneratorContext& context,
        const WaveConfig& config, int needsConfiguration, WaveState& state,
        WaveLookupTables& lookupTables, RandomSource& randomSource,
        LogSink& log) const;
};

/** Built-in wave renderer catalog used by WaveOption. */
extern Wave waveCatalog[];

/** Number of entries in waveCatalog. */
extern const int nWaveCatalogEntries;

/**
 * @param index Catalog index.
 * @return Wave at index, or NULL when out of range.
 */
Wave* waveByIndex(int index);

#endif
