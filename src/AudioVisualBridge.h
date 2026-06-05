/** @file
 * Audio-to-visual bridge and automatic scene-change ownership.
 */

#ifndef __AUDIO_VISUAL_BRIDGE_H
#define __AUDIO_VISUAL_BRIDGE_H

#include "AutoChangerStatusProvider.h"

#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
#include <memory>
#endif

class RuntimeCommandSink;
class AudioFrame;
class AcousticContext;
class AutoChanger;

/**
 * Processes frame audio before visual rendering and owns audio-driven policy.
 *
 * The bridge is the Application-owned boundary between acquired AudioFrame data
 * and visual runtime mutation. It analyzes supplied frames, updates the rolling
 * acoustic context, and runs AutoChanger when runtime commands are available.
 */
class AudioVisualBridge : public AutoChangerStatusProvider {
    int filterchainRefreshRequestedValue;
    AcousticContext& acousticContextValue;
    int minNoiseValue;

#ifndef CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
    std::unique_ptr<AutoChanger> autoChangerValue;
#endif

public:
    /**
     * Creates the audio-to-visual bridge.
     *
     * @param acousticContext_ Rolling acoustic context to update from analyzed
     *        frame metrics. The referenced object must outlive the bridge.
     * @param minNoise_ Noise-floor threshold used for AudioMetrics::noisy.
     * @param runtimeCommands_ Optional runtime command sink used by AutoChanger.
     *        When NULL, audio processing and analysis still run, but automatic
     *        scene changes are disabled.
     */
    AudioVisualBridge(AcousticContext& acousticContext_,
        int minNoise_, RuntimeCommandSink* runtimeCommands_ = 0);

    /** Releases bridge-owned automatic scene-change policy. */
    ~AudioVisualBridge();

    /**
     * Runs the audio side of one visual frame.
     *
     * Processes the supplied audio frame, publishes metrics on the frame,
     * updates the supplied AcousticContext, then runs AutoChanger if available.
     *
     * @param frame Current visual audio frame to process and analyze.
     */
    void runFrame(AudioFrame& frame);

    /**
     * Returns current automatic scene-change status text.
     *
     * @return Empty string when AutoChanger is disabled or unavailable;
     *         otherwise provider-owned text valid until the next status call.
     */
    virtual const char* autoChangerStatus() const;

    /**
     * @return Nonzero when audio-side option changes require rebuilding the
     *         visual filterchain before the next visual pass.
     */
    int filterchainRefreshRequested() const { return filterchainRefreshRequestedValue; }

    /**
     * Clears a pending visual filterchain refresh request after the caller has
     * rebuilt or refreshed the filterchain.
     */
    void clearFilterchainRefreshRequest() { filterchainRefreshRequestedValue = 0; }
};

#endif
