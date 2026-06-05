#include "cthugha.h"
#include "AudioAnalyzer.h"
#include "Configuration.h"

OptionInt sound_minnoise("minnoise", 0, SOUND_MINNOISE_MAX_EXCLUSIVE); /* quiet is below this */

AcousticContext acousticContext;

void configureAudioAnalyzer(const AutoChangeConfig& config) {
    sound_minnoise.setValue(config.minNoise);
}

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
