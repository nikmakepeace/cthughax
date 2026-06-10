/** @file
 * Runtime audio control port and default implementation.
 */

#ifndef CTHUGHA_RUNTIME_AUDIO_CONTROLS_H
#define CTHUGHA_RUNTIME_AUDIO_CONTROLS_H

#include "RuntimeCommandSink.h"

class AudioProcessingSelector;
class AcousticContext;
class MixerControls;
class Option;

/** Controls runtime audio-processing commands. */
class RuntimeAudioControls {
public:
    /** Destroys the audio controls interface. */
    virtual ~RuntimeAudioControls() { }

    /**
     * Changes sound processing by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changeSoundProcessingBy(int by) = 0;

    /**
     * Changes sound processing by name.
     *
     * @param to Choice text to select.
     */
    virtual void changeSoundProcessingTo(const char* to) = 0;

    /**
     * Sets fire detection sensitivity.
     *
     * @param sensitivity 0..100, where lower values suppress smaller bursts.
     */
    virtual void changeFireSensitivityTo(int sensitivity) = 0;

    /**
     * Sets the fire detection source.
     *
     * @param to Stable fire source name.
     */
    virtual void changeFireSourceTo(const char* to) = 0;

    /**
     * Attempts to change an audio-processing option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge audio-side effects into.
     * @return Nonzero when the option is the runtime audio-processing option.
     */
    virtual int changeAudioOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) = 0;

    /**
     * Attempts to change an audio-processing option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge audio-side effects into.
     * @return Nonzero when the option is the runtime audio-processing option.
     */
    virtual int changeAudioOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) = 0;
};

/** RuntimeAudioControls implementation backed by owned audio-processing state. */
class DefaultRuntimeAudioControls : public RuntimeAudioControls {
    AudioProcessingSelector& audioProcessingSelector;
    AcousticContext& acousticContext;
    MixerControls* mixerControls;

public:
    /**
     * Creates audio controls over an owned audio-processing selector.
     *
     * @param audioProcessingSelector_ Selector to mutate. The referenced object
     *        must outlive these controls.
     * @param acousticContext_ Acoustic context to mutate. The referenced object
     *        must outlive these controls.
     * @param mixerControls_ Optional mixer controls for OSS mixer panel rows.
     */
    explicit DefaultRuntimeAudioControls(
        AudioProcessingSelector& audioProcessingSelector_,
        AcousticContext& acousticContext_,
        MixerControls* mixerControls_ = 0);

    /**
     * Changes sound processing by relative offset.
     *
     * @param by Relative offset to apply.
     */
    virtual void changeSoundProcessingBy(int by);

    /**
     * Changes sound processing by name.
     *
     * @param to Choice text to select.
     */
    virtual void changeSoundProcessingTo(const char* to);

    /**
     * Sets fire detection sensitivity.
     *
     * @param sensitivity 0..100, where lower values suppress smaller bursts.
     */
    virtual void changeFireSensitivityTo(int sensitivity);

    /**
     * Sets the fire detection source.
     *
     * @param to Stable fire source name.
     */
    virtual void changeFireSourceTo(const char* to);

    /**
     * Attempts to change an audio-processing option by relative offset.
     *
     * @param option Option to inspect and possibly change.
     * @param by Relative offset to apply.
     * @param changes Change flags to merge audio-side effects into.
     * @return Nonzero when the option is the runtime audio-processing option.
     */
    virtual int changeAudioOptionBy(
        Option& option, int by, RuntimeChangeSet& changes);

    /**
     * Attempts to change an audio-processing option by value text.
     *
     * @param option Option to inspect and possibly change.
     * @param to Value text to select.
     * @param changes Change flags to merge audio-side effects into.
     * @return Nonzero when the option is the runtime audio-processing option.
     */
    virtual int changeAudioOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes);
};

#endif
