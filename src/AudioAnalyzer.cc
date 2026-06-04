#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaBuffer.h"
#include "AudioAnalyzer.h"
#include "CthughaDisplay.h"
#include "AudioFrame.h"
#include "Configuration.h"

#include <math.h>

OptionInt sound_minnoise("minnoise", 0, SOUND_MINNOISE_MAX_EXCLUSIVE); /* quiet is below this */

AudioAnalyzer audioAnalyzer;
AudioMetrics audioMetrics;
AcousticContext acousticContext;

void configureAudioAnalyzer(const AutoChangeConfig& config) {
    sound_minnoise.setValue(config.minNoise);
}

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) { }

AcousticContext::AcousticContext()
    : intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) { }

void AcousticContext::update(const AudioMetrics& metrics) {
    /* Rolling acoustic state lives here rather than in the frame metrics.
       Future context providers can add slower signals without changing the
       frame-local AudioMetrics contract. */
    int amplitude = metrics.amplitude;

    if (amplitude < lastAmplitudeValue - 9) /* ignore such a small decrease */
        amplitude = lastAmplitudeValue - 9;

    if (amplitude > lastAmplitudeValue)
        attackLevelValue += amplitude - lastAmplitudeValue;

    /* If the attack is over, fire at the intensity of the accumulated attack. */
    if (amplitude < lastAmplitudeValue) {
        fireValue = attackLevelValue;
        attackLevelValue = 0;

        if (fireValue > 0)
            CTH_TRACE("fire=%d amplitude=%d lastamp=%d\n", "sound fire",
                fireValue, amplitude, lastAmplitudeValue);
    } else
        fireValue = 0;

    lastAmplitudeValue = amplitude;
    intensityValue = intensityValue * 0.95 + (metrics.amplitude / 128.0) * 0.05;
    cumulativeFireLevelValue += fireValue;
}

double AcousticContext::intensity() const {
    return intensityValue;
}

int AcousticContext::fire() const {
    return fireValue;
}

int AcousticContext::cumulativeFireLevel() const {
    return cumulativeFireLevelValue;
}

void AcousticContext::resetCumulativeFireLevel() {
    cumulativeFireLevelValue = 0;
}

AudioAnalyzer::AudioAnalyzer() { }

AudioMetrics AudioAnalyzer::analyze(const char2* frame) {
    AudioMetrics metrics;
    int al = 0, ar = 0;

    /* get the amplitude of this sound frame (root mean squared) */
    char* d = (char*)frame;
    for (int i = 1024; i != 0; i--) {
        al += *d * *d;
        d++;
        ar += *d * *d;
        d++;
    }
    /* sqare root the mean */
    al = int(sqrt(double(al) / 1024));
    ar = int(sqrt(double(ar) / 1024));

    metrics.amplitude = (al + ar) / 2;
    metrics.amplitudeLeft = al;
    metrics.amplitudeRight = ar;

    /* check for silence */
    metrics.noisy = ((metrics.amplitudeLeft >= sound_minnoise)
        || (metrics.amplitudeRight >= sound_minnoise));

    return metrics;
}

void AudioAnalyzer::operator()() {
    static int debugReports = 0;
    AudioFrame* frame = audioFrameCurrent();

    audioMetrics = analyze(audioFrameRawData());
    acousticContext.update(audioMetrics);

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        debugReports++;
        CTH_DEBUG("audio analysis: amplitude=%d left=%d right=%d noisy=%d frame-samples=%d center-sample=%lld\n",
            audioMetrics.amplitude, audioMetrics.amplitudeLeft,
            audioMetrics.amplitudeRight, audioMetrics.noisy,
            frame ? frame->samples : 1024, frame ? frame->centerSample : 0);
    }
}
