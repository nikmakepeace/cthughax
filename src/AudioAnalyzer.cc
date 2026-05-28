#include "cthugha.h"
#include "Interface.h"
#include "display.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaBuffer.h"
#include "AudioAnalyzer.h"
#include "CthughaDisplay.h"
#include "AudioFrame.h"

#include <math.h>

OptionInt sound_minnoise("minnoise", 5, 256); /* quiet is below this */

AudioAnalyzer audioAnalyzer;
AudioAnalysis audioAnalysis;
AcousticContext acousticContext;

AudioAnalysis::AudioAnalysis()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) { }

AcousticContext::AcousticContext()
    : intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , fireLevelValue(0) { }

void AcousticContext::update(const AudioAnalysis& analysis) {
    /* Rolling acoustic state lives here rather than in the frame analysis.
       Future context providers can add slower signals without changing the
       frame-local AudioAnalysis contract. */
    int amplitude = analysis.amplitude;

    if (amplitude < lastAmplitudeValue - 9) /* ignore such a small decrease */
        amplitude = lastAmplitudeValue - 9;

    if (amplitude > lastAmplitudeValue)
        attackLevelValue += amplitude - lastAmplitudeValue;

    /* If the attack is over, fire at the intensity of the accumulated attack. */
    if (amplitude < lastAmplitudeValue) {
        fireValue = attackLevelValue;
        attackLevelValue = 0;

        if (fireValue > 0)
            CTH_DEBUG("sound fire: fire=%d amplitude=%d lastamp=%d\n",
                fireValue, amplitude, lastAmplitudeValue);
    } else
        fireValue = 0;

    lastAmplitudeValue = amplitude;
    intensityValue = intensityValue * 0.95 + (analysis.amplitude / 128.0) * 0.05;
    fireLevelValue += fireValue;
}

double AcousticContext::intensity() const {
    return intensityValue;
}

int AcousticContext::fire() const {
    return fireValue;
}

int AcousticContext::fireLevel() const {
    return fireLevelValue;
}

void AcousticContext::setFire(int fire) {
    fireValue = fire;
}

void AcousticContext::resetFire() {
    fireValue = 0;
}

void AcousticContext::resetFireLevel() {
    fireLevelValue = 0;
}

AudioAnalyzer::AudioAnalyzer() { }

AudioAnalysis AudioAnalyzer::analyze(const char2* frame) {
    AudioAnalysis analysis;
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

    analysis.amplitude = (al + ar) / 2;
    analysis.amplitudeLeft = al;
    analysis.amplitudeRight = ar;

    /* check for silence */
    analysis.noisy = ((analysis.amplitudeLeft >= sound_minnoise)
        || (analysis.amplitudeRight >= sound_minnoise));

    return analysis;
}

void AudioAnalyzer::operator()() {
    static int debugReports = 0;
    AudioFrame* frame = audioFrameCurrent();

    audioAnalysis = analyze(audioFrameData());
    acousticContext.update(audioAnalysis);

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        debugReports++;
        CTH_DEBUG("audio analysis: amplitude=%d left=%d right=%d noisy=%d frame-samples=%d center-sample=%lld\n",
            audioAnalysis.amplitude, audioAnalysis.amplitudeLeft,
            audioAnalysis.amplitudeRight, audioAnalysis.noisy,
            frame ? frame->samples : 1024, frame ? frame->centerSample : 0);
    }
}
