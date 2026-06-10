#include "AudioAnalyzer.h"
#include "configuration_defaults.h"
#include "ProcessServices.h"

#include <string.h>

AcousticContext::AcousticContext(LogSink* log_)
    : log(log_)
    , intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0)
    , fireSensitivityValue(100)
    , fireSourceValue(AcousticFireSourceRawAmplitude) { }

static int clampFireSensitivity(int sensitivity) {
    if (sensitivity < 0)
        return 0;
    if (sensitivity > 100)
        return 100;
    return sensitivity;
}

static int minimumFireForSensitivity(int sensitivity) {
    return (100 - clampFireSensitivity(sensitivity)) * 2;
}

static const char* fireSourceNameOf(AcousticFireSource source) {
    switch (source) {
    case AcousticFireSourceRawAmplitude:
        return AUDIO_ANALYSIS_FIRE_SOURCE_RAW_AMPLITUDE_TEXT;
    case AcousticFireSourceLowPass150HzAmplitude:
        return AUDIO_ANALYSIS_FIRE_SOURCE_LOW_PASS_150HZ_AMPLITUDE_TEXT;
    }

    return AUDIO_ANALYSIS_FIRE_SOURCE_RAW_AMPLITUDE_TEXT;
}

static int parseFireSourceName(
    const char* sourceName, AcousticFireSource* source) {
    if (sourceName == NULL)
        return 0;
    if (strcmp(sourceName, AUDIO_ANALYSIS_FIRE_SOURCE_RAW_AMPLITUDE_TEXT) == 0
        || strcmp(sourceName, "raw") == 0) {
        *source = AcousticFireSourceRawAmplitude;
        return 1;
    }
    if (strcmp(sourceName,
            AUDIO_ANALYSIS_FIRE_SOURCE_LOW_PASS_150HZ_AMPLITUDE_TEXT) == 0
        || strcmp(sourceName, "low-pass-150hz") == 0
        || strcmp(sourceName, "lowpass150hz") == 0) {
        *source = AcousticFireSourceLowPass150HzAmplitude;
        return 1;
    }

    return 0;
}

static int fireSourceAmplitude(
    const AudioMetrics& metrics, AcousticFireSource source) {
    return source == AcousticFireSourceLowPass150HzAmplitude
        ? metrics.lowPass150HzAmplitude
        : metrics.amplitude;
}

void AcousticContext::update(const AudioMetrics& metrics) {
    /* Rolling acoustic state lives here rather than in the frame metrics.
       Future context providers can add slower signals without changing the
       frame-local AudioMetrics contract. */
    int amplitude = fireSourceAmplitude(metrics, fireSourceValue);

    if (amplitude < lastAmplitudeValue - 9) /* ignore such a small decrease */
        amplitude = lastAmplitudeValue - 9;

    if (amplitude > lastAmplitudeValue)
        attackLevelValue += amplitude - lastAmplitudeValue;

    /* If the attack is over, fire at the intensity of the accumulated attack. */
    if (amplitude < lastAmplitudeValue) {
        fireValue = attackLevelValue;
        attackLevelValue = 0;
        if (fireValue <= minimumFireForSensitivity(fireSensitivityValue))
            fireValue = 0;

        if ((fireValue > 0) && (log != NULL))
            log->trace("sound fire", "fire=%d amplitude=%d lastamp=%d\n",
                fireValue, amplitude, lastAmplitudeValue);
    } else
        fireValue = 0;

    lastAmplitudeValue = amplitude;
    intensityValue = intensityValue * 0.95 + (metrics.amplitude / 128.0) * 0.05;
    cumulativeFireLevelValue += fireValue;
}

void AcousticContext::setFireSensitivity(int sensitivity) {
    fireSensitivityValue = clampFireSensitivity(sensitivity);
}

int AcousticContext::fireSensitivity() const {
    return fireSensitivityValue;
}

void AcousticContext::setFireSource(AcousticFireSource source) {
    if (fireSourceValue == source)
        return;

    fireSourceValue = source;
    lastAmplitudeValue = 0;
    attackLevelValue = 0;
    fireValue = 0;
}

int AcousticContext::setFireSource(const char* sourceName) {
    AcousticFireSource parsed = AcousticFireSourceRawAmplitude;
    if (!parseFireSourceName(sourceName, &parsed))
        return 0;

    setFireSource(parsed);
    return 1;
}

AcousticFireSource AcousticContext::fireSource() const {
    return fireSourceValue;
}

const char* AcousticContext::fireSourceName() const {
    return fireSourceNameOf(fireSourceValue);
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
