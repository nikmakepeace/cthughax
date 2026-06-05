#include "RuntimeConfigSelection.h"

#include <assert.h>

static void testReturnsConfiguredSelectionText() {
    Config config;
    config.scene.presentation = "display-roll";
    config.scene.flame = "flame-fire";
    config.scene.generalFlame = "general-soft";
    config.scene.border = "border-plain";
    config.scene.translation = "translation-swirl";
    config.scene.wave = "wave-line";
    config.scene.audioProcessing = "audio-fft";
    config.scene.table = "table-sine";
    config.scene.waveScale = "scale-wide";
    config.scene.palette = "palette-fire";
    config.scene.image = "image-logo";
    config.scene.object = "object-cube";
    config.scene.flashlight = "flashlight-on";

    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionDisplay)
        == "display-roll");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionFlame)
        == "flame-fire");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionGeneralFlame)
        == "general-soft");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionBorder)
        == "border-plain");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionTranslation)
        == "translation-swirl");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionWave)
        == "wave-line");
    assert(runtimeConfigSelectionText(
               config, RuntimeConfigSelectionAudioProcessing)
        == "audio-fft");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionTable)
        == "table-sine");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionWaveScale)
        == "scale-wide");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionPalette)
        == "palette-fire");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionImage)
        == "image-logo");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionObject)
        == "object-cube");
    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionFlashlight)
        == "flashlight-on");
}

static void testFallsBackForEmptySelectionText() {
    Config config;

    assert(runtimeConfigSelectionText(config, RuntimeConfigSelectionPalette)
        == "unknown");
    assert(runtimeConfigSelectionTextOrFallback(config,
               RuntimeConfigSelectionPalette, "fallback-palette")
        == "fallback-palette");
    assert(runtimeConfigSelectionTextOrFallback(config,
               RuntimeConfigSelectionPalette, "")
        == "unknown");
    assert(runtimeConfigSelectionTextOrFallback(config,
               RuntimeConfigSelectionPalette, NULL)
        == "unknown");
}

int main() {
    testReturnsConfiguredSelectionText();
    testFallsBackForEmptySelectionText();
    return 0;
}
