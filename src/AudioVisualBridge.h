// Audio-to-visual bridge.
//
// This layer consumes the current audio frame, updates acoustic context, and
// runs option-changing policy before visual buffer mutation begins.

#ifndef __AUDIO_VISUAL_BRIDGE_H
#define __AUDIO_VISUAL_BRIDGE_H

class RuntimeCommandSink;

class AudioVisualBridge {
    int filterchainRefreshRequestedValue;
    RuntimeCommandSink* runtimeCommands;

public:
    /**
     * Creates the audio-to-visual bridge.
     *
     * @param runtimeCommands_ Optional runtime command sink used by AutoChanger.
     *        When NULL, audio processing and analysis still run, but automatic
     *        scene changes are disabled.
     */
    AudioVisualBridge(RuntimeCommandSink* runtimeCommands_ = 0);
    ~AudioVisualBridge();

    /**
     * Runs the audio side of one visual frame.
     *
     * Processes the current audio frame, publishes AudioFrame metrics, updates
     * AcousticContext, then runs AutoChanger if available.
     * Called after audioFrameTick() and before visual filterchain mutation.
     */
    void runFrame();

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
