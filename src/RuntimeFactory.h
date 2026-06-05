/** @file
 * Startup-time runtime composition helpers.
 */

#ifndef __RUNTIME_FACTORY_H
#define __RUNTIME_FACTORY_H

#include "PcmSourceFactory.h"
#include "AudioSettings.h"

class Environment {
public:
    int ossInputAvailable;
    int ossOutputAvailable;
    int pulseOutputAvailable;

    /** Creates an environment with no detected backend availability. */
    Environment();

    /**
     * Detects audio backend availability for this process.
     *
     * @return Environment describing available input/output backends.
     */
    static Environment detect();
};

class RuntimeFactory {
    AudioSettings settings;
    Environment environment;
    int visualMaxDimension;
    PcmSourceFactory pcmSourceFactory;

public:
    /**
     * Creates a factory for one startup/runtime-selection pass.
     *
     * @param settings Snapshot of audio options used to select input/output.
     * @param environment Detected backend availability for this process.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Passed to DSP/audio-window constructors.
     */
    RuntimeFactory(const AudioSettings& settings, const Environment& environment,
        int visualMaxDimension);

    /**
     * @return Newly allocated audio input wrapper. Caller owns the returned pointer.
     */
    AudioInput* createAudioInput() const;

    /**
     * @return Newly allocated audio output backend. Caller owns the returned pointer.
     */
    AudioOutput* createAudioOutput() const;

    /**
     * @return Newly allocated input processor. Caller owns the returned pointer.
     */
    AudioInputProcessor* createAudioProcessor() const;

    /**
     * @return Strategy selected from the current settings and environment.
     */
    AudioSourceStrategy selectAudioSourceStrategy() const;
};

#endif
