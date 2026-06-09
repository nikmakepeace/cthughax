#include "DisplayPresentationOptions.h"

#include "Configuration.h"

DisplayPresentationSettings::DisplayPresentationSettings()
    : maxFramesPerSecond("maxFPS", 0)
    , showFPS("show-fps", 0)
    , zoom("zoom", 0, ZOOM_MODE_MAX_EXCLUSIVE) {
}

void DisplayPresentationSettings::configure(const DisplayConfig& config) {
    maxFramesPerSecond.setValue(config.maxFramesPerSecond);
    showFPS.setValue(config.showFpsEnabled);
    zoom.setValue(config.zoomMode);
}
