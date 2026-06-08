// Legacy loader adapter for native Scene palette catalogs.

#include "LegacyScenePaletteCatalogAdapter.h"

#include "EffectControl.h"
#include "PaletteEntry.h"
#include "ScenePaletteCatalog.h"

void loadScenePaletteCatalogFromLegacy(
    EffectControl& paletteOption, ScenePaletteCatalog& catalog) {
    catalog.clear();

    for (int i = 0; i < paletteOption.getNEntries(); i++) {
        PaletteEntry* entry = dynamic_cast<PaletteEntry*>(paletteOption[i]);
        if (entry != 0)
            catalog.addChoice(*entry, entry->inUse());
    }
}
