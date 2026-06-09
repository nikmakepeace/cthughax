/** @file
 * Immutable per-frame audio analysis values for visual consumers.
 */

#include "AudioAnalysisSnapshot.h"

AudioAnalysisSnapshot::AudioAnalysisSnapshot()
    : metricsValue()
    , intensityValue(0.0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) { }

AudioAnalysisSnapshot::AudioAnalysisSnapshot(const AudioMetrics& metrics,
    double intensity, int fire, int cumulativeFireLevel)
    : metricsValue(metrics)
    , intensityValue(intensity)
    , fireValue(fire)
    , cumulativeFireLevelValue(cumulativeFireLevel) { }

const AudioMetrics& AudioAnalysisSnapshot::metrics() const {
    return metricsValue;
}

int AudioAnalysisSnapshot::amplitude() const {
    return metricsValue.amplitude;
}

int AudioAnalysisSnapshot::amplitudeLeft() const {
    return metricsValue.amplitudeLeft;
}

int AudioAnalysisSnapshot::amplitudeRight() const {
    return metricsValue.amplitudeRight;
}

int AudioAnalysisSnapshot::noisy() const {
    return metricsValue.noisy;
}

double AudioAnalysisSnapshot::intensity() const {
    return intensityValue;
}

int AudioAnalysisSnapshot::fire() const {
    return fireValue;
}

int AudioAnalysisSnapshot::cumulativeFireLevel() const {
    return cumulativeFireLevelValue;
}
