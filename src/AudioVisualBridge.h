// Audio-to-visual bridge.
//
// This layer consumes the current audio frame, updates acoustic context, and
// runs option-changing policy before visual buffer mutation begins.

#ifndef __AUDIO_VISUAL_BRIDGE_H
#define __AUDIO_VISUAL_BRIDGE_H

class RuntimeCommandSink;
class AudioFrame;
class AcousticContext;

class AudioVisualBridge {
    int filterchainRefreshRequestedValue;
    AcousticContext& acousticContextValue;
    int minNoiseValue;
    RuntimeCommandSink* runtimeCommands;

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
