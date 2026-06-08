// Legacy loader adapter for native Scene palette catalogs.

#ifndef CTHUGHA_LEGACY_SCENE_PALETTE_CATALOG_ADAPTER_H
#define CTHUGHA_LEGACY_SCENE_PALETTE_CATALOG_ADAPTER_H

class EffectControl;
class ScenePaletteCatalog;

/**
 * Copies legacy palette option entries into a native Scene palette catalog.
 *
 * This quarantines the remaining palette option loader outside the Scene
 * selection factory while palette file loading is still legacy.
 *
 * @param paletteOption Legacy palette option populated by load_palettes().
 * @param catalog Native catalog to replace with copied entries.
 */
void loadScenePaletteCatalogFromLegacy(
    EffectControl& paletteOption, ScenePaletteCatalog& catalog);

#endif
