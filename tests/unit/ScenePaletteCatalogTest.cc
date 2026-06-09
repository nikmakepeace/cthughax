#include "ScenePaletteCatalog.h"

#include "PaletteEntry.h"

#include <assert.h>
#include <string.h>

int cth_log_error(const char*, ...) { return 0; }

static void testCatalogOwnsCopiedPaletteEntries() {
    PaletteEntry source("fixture", "Fixture Palette");
    source.colors().setColor(3, 10, 20, 30);
    strcpy(source.sourcePath, "/tmp/source.map");
    source.setMetadataName("Display Name");
    strcpy(source.metadataSet, "warm,bright");
    strcpy(source.metadataEnergy, "high");
    source.metadataSetCount = 2;
    strcpy(source.metadataSets[0], "warm");
    strcpy(source.metadataSets[1], "bright");
    source.metadataEnergyCount = 1;
    strcpy(source.metadataEnergies[0], "high");

    ScenePaletteCatalog catalog;
    catalog.addChoice(source, 1);

    source.colors().setColor(3, 90, 80, 70);
    strcpy(source.metadataSets[0], "mutated");

    assert(catalog.entryCount() == 1);
    assert(catalog.inUseAt(0) == 1);

    const PaletteEntry* copy = catalog.paletteAt(0);
    assert(copy != 0);
    assert(strcmp(copy->Name(), "fixture") == 0);
    assert(strcmp(copy->Desc(), "Display Name") == 0);
    assert(copy->colors().component(3, 0) == 10);
    assert(copy->colors().component(3, 1) == 20);
    assert(copy->colors().component(3, 2) == 30);
    assert(strcmp(copy->sourcePath, "/tmp/source.map") == 0);
    assert(strcmp(copy->metadataName, "Display Name") == 0);
    assert(strcmp(copy->metadataSet, "warm,bright") == 0);
    assert(strcmp(copy->metadataEnergy, "high") == 0);
    assert(copy->metadataSetCount == 2);
    assert(strcmp(copy->metadataSets[0], "warm") == 0);
    assert(strcmp(copy->metadataSets[1], "bright") == 0);
    assert(copy->metadataEnergyCount == 1);
    assert(strcmp(copy->metadataEnergies[0], "high") == 0);
}

static void testOutOfRangeEntriesAreEmpty() {
    ScenePaletteCatalog catalog;

    assert(catalog.entryCount() == 0);
    assert(catalog.paletteAt(-1) == 0);
    assert(catalog.paletteAt(0) == 0);
    assert(catalog.inUseAt(-1) == 0);
    assert(catalog.inUseAt(0) == 0);
}

int main() {
    testCatalogOwnsCopiedPaletteEntries();
    testOutOfRangeEntriesAreEmpty();
    return 0;
}
