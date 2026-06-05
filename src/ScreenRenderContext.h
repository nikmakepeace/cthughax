/** @file
 * Explicit source, destination, timing, and audio context for screen renderers.
 */

#ifndef __SCREEN_RENDER_CONTEXT_H
#define __SCREEN_RENDER_CONTEXT_H

#include "AudioTypes.h"
#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"

class AudioFrame;
class AcousticContext;
class VideoFrameContext;
struct AudioMetrics;

/**
 * Borrowed inputs for one classic screen/presentation renderer invocation.
 */
class ScreenRenderContext {
    const IndexedFrame* sourceValue;
    IndexedDisplayFrame* destinationValue;
    unsigned char* destinationPixelsValue;
    int destinationWidthValue;
    int destinationHeightValue;
    int destinationPitchValue;
    const AudioFrame* audioFrameValue;
    const char2* rawAudioDataValue;
    const char2* processedWaveDataValue;
    const AudioMetrics* audioMetricsValue;
    const AcousticContext* acousticContextValue;
    double frameTimeSecondsValue;
    double deltaTimeSecondsValue;
    double framesPerSecondValue;

public:
    /**
     * Creates a render context without audio inputs.
     *
     * @param source Indexed source frame to transform.
     * @param destination Indexed destination frame to write.
     * @param frameTimeSeconds Visual-frame timestamp in seconds.
     * @param deltaTimeSeconds Seconds since the previous visual frame.
     * @param framesPerSecond Current presentation-frame rate estimate.
     */
    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond);

    /**
     * Creates a render context with explicit audio inputs.
     *
     * @param source Indexed source frame to transform.
     * @param destination Indexed destination frame to write.
     * @param frameTimeSeconds Visual-frame timestamp in seconds.
     * @param deltaTimeSeconds Seconds since the previous visual frame.
     * @param framesPerSecond Current presentation-frame rate estimate.
     * @param frameContext Borrowed per-frame audio context.
     */
    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
        const VideoFrameContext& frameContext);

    /**
     * Creates a render context with an explicit destination pixel window.
     *
     * @param source Indexed source frame to transform.
     * @param destination Indexed destination frame associated with the output.
     * @param destinationPixels First writable destination pixel.
     * @param destinationWidth Width of the writable destination window.
     * @param destinationHeight Height of the writable destination window.
     * @param destinationPitch Bytes between destination rows.
     * @param frameTimeSeconds Visual-frame timestamp in seconds.
     * @param deltaTimeSeconds Seconds since the previous visual frame.
     * @param framesPerSecond Current presentation-frame rate estimate.
     */
    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        unsigned char* destinationPixels, int destinationWidth, int destinationHeight,
        int destinationPitch, double frameTimeSeconds, double deltaTimeSeconds,
        double framesPerSecond);

    /** @return Source indexed frame being transformed. */
    const IndexedFrame& source() const;

    /** @return Destination indexed frame being written. */
    IndexedDisplayFrame& destination() const;

    /** @return First source pixel. */
    const unsigned char* sourcePixels() const;

    /** @return First source pixel on row y. */
    const unsigned char* sourceLine(int y) const;

    /** @return Source width in pixels. */
    int sourceWidth() const;

    /** @return Source height in pixels. */
    int sourceHeight() const;

    /** @return Source row pitch in bytes. */
    int sourcePitch() const;

    /** @return First writable destination pixel. */
    unsigned char* destinationPixels();

    /** @return First writable destination pixel on row y. */
    unsigned char* destinationLine(int y);

    /** @return Destination writable width in pixels. */
    int destinationWidth() const;

    /** @return Destination writable height in pixels. */
    int destinationHeight() const;

    /** @return Destination row pitch in bytes. */
    int destinationPitch() const;

    /** @return Audio frame for this visual frame, or NULL. */
    const AudioFrame* audioFrame() const;

    /** @return Raw signed 8-bit stereo audio samples, or NULL. */
    const char2* rawAudioData() const;

    /** @return Processed signed 8-bit stereo samples, or NULL. */
    const char2* processedWaveData() const;

    /** @return Metrics derived from the current audio frame, or NULL. */
    const AudioMetrics* audioMetrics() const;

    /** @return Rolling acoustic state for this visual frame, or NULL. */
    const AcousticContext* acousticContext() const;

    /** @return Visual-frame timestamp in seconds. */
    double frameTimeSeconds() const;

    /** @return Seconds since the previous visual frame. */
    double deltaTimeSeconds() const;

    /** @return Current presentation-frame rate estimate. */
    double framesPerSecond() const;
};

#endif
