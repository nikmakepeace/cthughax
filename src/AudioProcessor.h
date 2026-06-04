// -*- c++ -*-

#ifndef __AUDIO_PROCESSOR_H
#define __AUDIO_PROCESSOR_H

#include "cthugha.h"
#include "EffectControl.h"
#include "Option.h"

#include <string>

struct SceneConfig;

class AudioProcessingOption : public Option {
    EffectChoiceList& entries;
    std::string initialEntry;

    int entryCount() const;
    int optNr(const char* name) const;

public:
    /**
     * Creates the audio-processing option wrapper.
     *
     * @param name Option name used by command-line and ini parsing.
     * @param entries_ Available processing modes. The list must outlive this option.
     */
    AudioProcessingOption(const char* name, EffectChoiceList& entries);

    /**
     * Sets the startup processing mode text.
     *
     * @param entry Entry name or numeric index. NULL/empty means choose a random
     *        processing mode when changeToInitial() runs.
     */
    void setInitialEntry(const char* entry);

    /**
     * Applies the startup processing mode captured by setInitialEntry().
     */
    void changeToInitial();

    /**
     * Rotates the current processing mode.
     *
     * @param by Signed entry offset.
     */
    virtual void change(int by);

    /**
     * Selects a processing mode by name or numeric index.
     *
     * @param to Entry name, numeric index text, or NULL/empty to choose randomly.
     */
    virtual void change(const char* to);

    /**
     * @return Name of the current processing mode, or "unknown" if invalid.
     */
    virtual const char* text() const;

    /**
     * Processes the current audio frame into processedWaveData.
     *
     * @return Result from the selected processing entry.
     */
    int process();
};

extern AudioProcessingOption audioProcessing;

void configureAudioProcessing(const SceneConfig& config);

#endif
