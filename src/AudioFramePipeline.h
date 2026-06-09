/** @file
 * Per-frame audio processing pipeline for visualization frames.
 */

#ifndef CTHUGHA_AUDIO_FRAME_PIPELINE_H
#define CTHUGHA_AUDIO_FRAME_PIPELINE_H

class AcousticContext;
class AudioFrame;
class AudioProcessingSelector;
class AudioProcessor;
class LogSink;
class SecondsClock;

/**
 * Port for converting acquired audio frames into analyzed visualization input.
 */
class AudioFramePipeline {
public:
    /** Destroys the audio frame pipeline interface. */
    virtual ~AudioFramePipeline() { }

    /**
     * Processes and analyzes one visual audio frame.
     *
     * @param frame Frame to process. Implementations populate processed audio,
     *        frame-local metrics, and any rolling acoustic state they own.
     */
    virtual void processFrame(AudioFrame& frame) = 0;

    /**
     * Returns rolling acoustic state updated by processFrame().
     *
     * @return Acoustic context owned outside the pipeline.
     */
    virtual AcousticContext& acousticContext() = 0;

    /**
     * Returns rolling acoustic state updated by processFrame().
     *
     * @return Acoustic context owned outside the pipeline.
     */
    virtual const AcousticContext& acousticContext() const = 0;

    /** @return Number of bounded debug reports emitted by this pipeline. */
    virtual int debugReportCount() const = 0;
};

/**
 * Default audio frame pipeline backed by the runtime processing selector.
 */
class DefaultAudioFramePipeline : public AudioFramePipeline {
    AcousticContext& acousticContextValue;
    AudioProcessingSelector& audioProcessingSelectorValue;
    AudioProcessor& audioProcessorValue;
    SecondsClock& clock;
    LogSink& log;
    int minNoiseValue;
    int debugReportsValue;

public:
    /**
     * Creates the default audio frame pipeline.
     *
     * @param acousticContext_ Rolling acoustic context to update from analyzed
     *        frame metrics. The referenced object must outlive the pipeline.
     * @param audioProcessingSelector_ Selector used to process frame audio.
     *        The referenced object must outlive the pipeline.
     * @param audioProcessor_ Processor used for frame analysis. The referenced
     *        object must outlive the pipeline.
     * @param clock_ Clock used for trace timing. The referenced object must
     *        outlive the pipeline.
     * @param log_ Sink for diagnostics. The referenced object must outlive the
     *        pipeline.
     * @param minNoise_ Noise-floor threshold used for AudioMetrics::noisy.
     */
    DefaultAudioFramePipeline(AcousticContext& acousticContext_,
        AudioProcessingSelector& audioProcessingSelector_,
        AudioProcessor& audioProcessor_, SecondsClock& clock_, LogSink& log_,
        int minNoise_);

    /** Processes and analyzes one visual audio frame. */
    virtual void processFrame(AudioFrame& frame);

    /** @return Rolling acoustic context updated by processFrame(). */
    virtual AcousticContext& acousticContext();

    /** @return Rolling acoustic context updated by processFrame(). */
    virtual const AcousticContext& acousticContext() const;

    /** @return Number of bounded debug reports emitted by this pipeline. */
    virtual int debugReportCount() const;
};

#endif
