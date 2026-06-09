/** @file
 * Runtime-selected audio processing state and selector.
 */

#ifndef CTHUGHA_AUDIO_PROCESSING_H
#define CTHUGHA_AUDIO_PROCESSING_H

#include "Option.h"

#include <string>

struct SceneConfig;
class AudioFrame;
class AudioProcessor;
class AudioProcessingSelector;
class LogSink;
class RandomSource;

/** Audio processing modes available for visualization frames. */
enum AudioProcessingMode {
    AudioProcessingModeNone = 0,
    AudioProcessingModeFilter1,
    AudioProcessingModeFilter2,
    AudioProcessingModeFft
};

/**
 * Owned selected audio-processing mode.
 *
 * This state is application-owned. Legacy option panels and runtime command
 * routing mutate it through AudioProcessingSelector rather than through a
 * process-global Option.
 */
class AudioProcessingState {
    RandomSource& randomSource;
    int selectedMode;
    std::string initialEntry;

    int entryCount() const;
    int optNr(const char* name) const;

public:
    /**
     * Creates audio processing state with the default "none" mode selected.
     *
     * @param randomSource_ Random source used when startup/runtime selection
     *        asks for an unspecified or unknown processing mode. The referenced
     *        object must outlive this state.
     */
    explicit AudioProcessingState(RandomSource& randomSource_);

    /**
     * Captures the startup processing mode text.
     *
     * @param entry Entry name or numeric index. NULL/empty means choose a
     *        random processing mode when changeToInitial() runs.
     */
    void setInitialEntry(const char* entry);

    /** Applies the startup processing mode captured by setInitialEntry(). */
    void changeToInitial();

    /**
     * Rotates the selected processing mode.
     *
     * @param by Signed entry offset.
     */
    void changeBy(int by);

    /**
     * Selects a processing mode by name or numeric index.
     *
     * @param to Entry name, numeric index text, or NULL/empty to choose random.
     */
    void changeTo(const char* to);

    /**
     * @return Name of the selected processing mode, or "unknown" if invalid.
     */
    const char* text() const;

    /**
     * @return Selected processing mode. Invalid internal values map to none.
     */
    AudioProcessingMode mode() const;
};

/**
 * Option adapter for legacy panels and generic option command routing.
 */
class AudioProcessingOption : public Option {
    AudioProcessingSelector& selector;

public:
    /**
     * Creates an option adapter over an owned selector.
     *
     * @param selector_ Selector to mutate and read. The referenced object must
     *        outlive this adapter.
     */
    explicit AudioProcessingOption(AudioProcessingSelector& selector_);

    /**
     * Rotates the selected processing mode.
     *
     * @param by Signed entry offset.
     */
    virtual void change(int by);

    /**
     * Selects a processing mode by name or numeric index.
     *
     * @param to Entry name, numeric index text, or NULL/empty to choose random.
     */
    virtual void change(const char* to);

    /**
     * @return Name of the selected processing mode.
     */
    virtual const char* text() const;
};

/**
 * Applies the selected processing mode to frames.
 */
class AudioProcessingSelector {
    AudioProcessingState& state;
    AudioProcessor& processor;
    LogSink& log;
    AudioProcessingOption optionValue;

public:
    /**
     * Creates a selector over owned state and processor.
     *
     * @param state_ Selected processing state. The referenced object must
     *        outlive this selector.
     * @param processor_ Audio processor used to apply selected modes. The
     *        referenced object must outlive this selector.
     * @param log_ Sink for processing diagnostics. The referenced object must
     *        outlive this selector.
     */
    AudioProcessingSelector(AudioProcessingState& state_,
        AudioProcessor& processor_, LogSink& log_);

    /**
     * Applies startup scene audio-processing selection.
     *
     * @param config Startup scene configuration.
     */
    void configureStartup(const SceneConfig& config);

    /**
     * Rotates the selected processing mode.
     *
     * @param by Signed entry offset.
     */
    void changeBy(int by);

    /**
     * Selects a processing mode by name or numeric index.
     *
     * @param to Entry name, numeric index text, or NULL/empty to choose random.
     */
    void changeTo(const char* to);

    /**
     * @return Name of the selected processing mode.
     */
    const char* text() const;

    /**
     * @return Selected processing mode.
     */
    AudioProcessingMode mode() const;

    /**
     * Processes a supplied audio frame into processedWaveData.
     *
     * @param frame Frame to process.
     * @return Nonzero when a processing mode was applied.
     */
    int process(AudioFrame& frame);

    /**
     * @return Legacy Option adapter for interface panels and generic commands.
     */
    AudioProcessingOption& option();

    /**
     * @return Legacy Option adapter for read-only callers.
     */
    const AudioProcessingOption& option() const;
};

#endif
