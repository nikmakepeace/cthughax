// Scene visual selection bridge over current legacy global controls.

#include "LegacyGlobalSceneSelectionFactory.h"

#include "BorderOption.h"
#include "FlashlightOption.h"
#include "FlameOptions.h"
#include "Image.h"
#include "LegacySceneSelectionAdapters.h"
#include "PaletteOption.h"
#include "TranslationOption.h"
#include "WaveOptions.h"

std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacyGlobalSceneSelectionAdapters(
    ImageOption& images, const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations) {
    return createLegacySceneSelectionAdapters(flame, flameGeneral, wave,
        waveScale, table, object, translation, palette, border, flashlight,
        images, waveObjects, imageCatalog, paletteCatalog, translations);
}
