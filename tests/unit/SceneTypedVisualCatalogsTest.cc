#include "SceneTypedVisualCatalogs.h"

#include "EffectChoiceLoader.h"
#include "Image.h"
#include "PaletteEntry.h"
#include "ProcessServices.h"
#include "SceneImageCatalog.h"
#include "ScenePaletteCatalog.h"
#include "SceneTranslationCatalog.h"
#include "SceneWaveObjectCatalog.h"
#include "pcx.h"
#include "png.h"

#include <cassert>
#include <cstring>
#include <stdint.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceContextLoader, void*) {
    return 0;
}

class RecordingLock : public SceneChoiceLock {
public:
    int enabledValue;

    RecordingLock()
        : enabledValue(0) { }

    virtual int enabled() const { return enabledValue; }

    virtual void change(const char* to) {
        enabledValue = std::strcmp(to, "on") == 0;
    }
};

class FixedRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) { return 1; }
};

static const Flame* flameIdentity(uintptr_t value) {
    return reinterpret_cast<const Flame*>(value);
}

static Wave* waveIdentity(uintptr_t value) {
    return reinterpret_cast<Wave*>(value);
}

static void testFlameCatalogOwnsTypedChoices() {
    const Flame* clear = flameIdentity(0x1000);
    const Flame* spark = flameIdentity(0x2000);
    SceneFlameChoiceCatalog catalog("flame", new RecordingLock());
    catalog.addChoice(clear, "Clear", 0);
    SceneFlameChoice& sparkChoice = catalog.addChoice(spark, "Spark", 1);

    assert(std::strcmp(catalog.optionName(), "flame") == 0);
    assert(catalog.entryCount() == 2);
    assert(catalog.choiceAt(-1) == 0);
    assert(catalog.choiceAt(2) == 0);
    assert(catalog.choiceAt(0)->sameName("clear") != 0);
    assert(catalog.choiceAt(1)->sameName("SPARK trailing") != 0);
    assert(catalog.choiceAt(0)->inUse() == 0);
    assert(sparkChoice.flame() == spark);

    catalog.choiceAt(0)->setUse(1);
    assert(catalog.choiceAt(0)->inUse() == 1);
}

static void testFlameSelectionReturnsTypedFlame() {
    const Flame* clear = flameIdentity(0x3000);
    const Flame* spark = flameIdentity(0x4000);
    SceneFlameChoiceCatalog* catalog
        = new SceneFlameChoiceCatalog("flame", new RecordingLock());
    catalog->addChoice(clear, "Clear", 1);
    catalog->addChoice(spark, "Spark", 1);
    SceneFlameChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    assert(selection.currentFlame() == clear);
    selection.change("spark", randomSource);
    assert(selection.currentValue() == 1);
    assert(selection.currentFlame() == spark);
}

static void testWaveCatalogOwnsTypedChoices() {
    Wave* dot = waveIdentity(0x5000);
    Wave* none = waveIdentity(0x6000);
    SceneWaveChoiceCatalog catalog("wave", new RecordingLock());
    catalog.addChoice(dot, "DotHor", 1);
    SceneWaveChoice& noneChoice = catalog.addChoice(none, "None", 0);

    assert(std::strcmp(catalog.optionName(), "wave") == 0);
    assert(catalog.entryCount() == 2);
    assert(catalog.choiceAt(0)->sameName("dothor") != 0);
    assert(catalog.choiceAt(1)->sameName("NONE trailing") != 0);
    assert(catalog.choiceAt(1)->inUse() == 0);
    assert(noneChoice.wave() == none);
}

static void testWaveSelectionReturnsTypedWave() {
    Wave* dot = waveIdentity(0x7000);
    Wave* none = waveIdentity(0x8000);
    SceneWaveChoiceCatalog* catalog
        = new SceneWaveChoiceCatalog("wave", new RecordingLock());
    catalog->addChoice(dot, "DotHor", 1);
    catalog->addChoice(none, "None", 1);
    SceneWaveChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    assert(selection.currentWave() == dot);
    selection.change("none", randomSource);
    assert(selection.currentValue() == 1);
    assert(selection.currentWave() == none);

    selection.toggleLock();
    assert(selection.lockEnabled() == 1);
    assert(catalog->lock().enabled() == 1);
}

static void testWaveObjectCatalogCopiesObjectData() {
    WObject source[] = {
        { { 0, 1, 2 }, { 3, 4, 5 } },
        { { -1, -1, -1 }, { -1, -1, -1 } }
    };

    SceneWaveObjectChoiceCatalog catalog("object", new RecordingLock());
    SceneWaveObjectChoice& choice = catalog.addChoice("shape", source, 1);
    source[0][0][0] = 99;

    assert(std::strcmp(catalog.optionName(), "object") == 0);
    assert(catalog.entryCount() == 1);
    assert(catalog.choiceAt(0)->sameName("SHAPE trailing") != 0);

    WObject* copied = choice.object();
    assert(copied != source);
    assert(copied[0][0][0] == 0);
    assert(copied[0][1][2] == 5);
    assert(copied[1][0][0] == -1);
    assert(copied[1][1][2] == -1);
}

static void testWaveObjectSelectionReturnsOwnedObject() {
    WObject first[] = {
        { { 1, 0, 0 }, { 2, 0, 0 } },
        { { -1, -1, -1 }, { -1, -1, -1 } }
    };
    WObject second[] = {
        { { 0, 1, 0 }, { 0, 2, 0 } },
        { { -1, -1, -1 }, { -1, -1, -1 } }
    };
    SceneWaveObjectChoiceCatalog* catalog
        = new SceneWaveObjectChoiceCatalog("object", new RecordingLock());
    catalog->addChoice("first", first, 1);
    catalog->addChoice("second", second, 1);
    SceneWaveObjectChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    assert(selection.currentObject()[0][0][0] == 1);
    selection.change("second", randomSource);
    WObject* selected = selection.currentObject();
    assert(selection.currentValue() == 1);
    assert(selected != second);
    assert(selected[0][0][1] == 1);
    assert(selected[0][1][1] == 2);
}

static void testTranslationCatalogCopiesTableData() {
    int sourceData[] = { 0, 1, 2, 3 };
    SceneTranslationChoiceCatalog catalog("translate", new RecordingLock());
    SceneTranslationChoice& choice = catalog.addChoice(
        TranslationTable("warp", sourceData, 2, 2), 1);
    sourceData[1] = 99;

    assert(std::strcmp(catalog.optionName(), "translate") == 0);
    assert(catalog.entryCount() == 1);
    assert(catalog.choiceAt(0)->sameName("WARP trailing") != 0);

    TranslationTable table = choice.table();
    assert(std::strcmp(table.name(), "warp") == 0);
    assert(table.ready() != 0);
    assert(table.width() == 2);
    assert(table.height() == 2);
    assert(table.data() != sourceData);
    assert(table.data()[1] == 1);
}

static void testTranslationSelectionReturnsOwnedTable() {
    int firstData[] = { 0, 1, 2, 3 };
    int secondData[] = { 3, 2, 1, 0 };
    SceneTranslationChoiceCatalog* catalog
        = new SceneTranslationChoiceCatalog("translate", new RecordingLock());
    catalog->addChoice(TranslationTable("none", 0, 0, 0), 0);
    catalog->addChoice(TranslationTable("twist", firstData, 2, 2), 1);
    catalog->addChoice(TranslationTable("mirror", secondData, 2, 2), 1);
    SceneTranslationChoiceSelection selection(catalog, 1);
    FixedRandomSource randomSource;

    assert(std::strcmp(selection.currentTranslationTable().name(), "twist")
        == 0);
    selection.change("mirror", randomSource);
    TranslationTable table = selection.currentTranslationTable();
    assert(selection.currentValue() == 2);
    assert(std::strcmp(table.name(), "mirror") == 0);
    assert(table.data()[0] == 3);
}

static void testPaletteCatalogCopiesEntry() {
    PaletteEntry source("warm", "Warm Palette");
    source.colors().setColor(5, 10, 20, 30);
    std::strcpy(source.sourcePath, "resources/map/warm.map");
    std::strcpy(source.metadataName, "Warm Test");
    std::strcpy(source.metadataSet, "classic");
    source.metadataSetCount = 1;
    std::strcpy(source.metadataSets[0], "classic");
    std::strcpy(source.metadataEnergy, "high");
    source.metadataEnergyCount = 1;
    std::strcpy(source.metadataEnergies[0], "high");

    ScenePaletteChoiceCatalog catalog("palette", new RecordingLock());
    ScenePaletteChoice& choice = catalog.addChoice(source, 1);
    source.colors().setColor(5, 100, 100, 100);
    std::strcpy(source.metadataSets[0], "mutated");

    assert(std::strcmp(catalog.optionName(), "palette") == 0);
    assert(catalog.entryCount() == 1);
    assert(catalog.choiceAt(0)->sameName("WARM trailing") != 0);

    PaletteEntry* copied = choice.paletteEntry();
    assert(copied != &source);
    assert(std::strcmp(copied->Name(), "warm") == 0);
    assert(std::strcmp(copied->Desc(), "Warm Palette") == 0);
    assert(copied->colors().component(5, 0) == 10);
    assert(copied->colors().component(5, 1) == 20);
    assert(copied->colors().component(5, 2) == 30);
    assert(std::strcmp(copied->sourcePath, "resources/map/warm.map") == 0);
    assert(std::strcmp(copied->metadataName, "Warm Test") == 0);
    assert(copied->metadataSetCount == 1);
    assert(std::strcmp(copied->metadataSets[0], "classic") == 0);
    assert(copied->metadataEnergyCount == 1);
    assert(std::strcmp(copied->metadataEnergies[0], "high") == 0);
}

static void testPaletteSelectionReturnsOwnedEntry() {
    PaletteEntry first("warm", "Warm Palette");
    first.colors().setColor(1, 1, 2, 3);
    PaletteEntry second("cool", "Cool Palette");
    second.colors().setColor(1, 4, 5, 6);
    ScenePaletteChoiceCatalog* catalog
        = new ScenePaletteChoiceCatalog("palette", new RecordingLock());
    catalog->addChoice(first, 1);
    catalog->addChoice(second, 1);
    ScenePaletteChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    assert(std::strcmp(selection.currentPaletteEntry()->Name(), "warm") == 0);
    selection.change("cool", randomSource);
    PaletteEntry* selected = selection.currentPaletteEntry();
    assert(selection.currentValue() == 1);
    assert(std::strcmp(selected->Name(), "cool") == 0);
    assert(selected->colors().component(1, 2) == 6);
}

static void testPaletteSelectionCanReplaceAndAppendEntries() {
    PaletteEntry first("warm", "Warm Palette");
    first.colors().setColor(1, 1, 2, 3);
    PaletteEntry replacement("warm", "Warmer Palette");
    replacement.colors().setColor(1, 7, 8, 9);
    PaletteEntry appended("random.1", "Random Palette");
    appended.colors().setColor(1, 11, 12, 13);
    ScenePaletteChoiceCatalog* catalog
        = new ScenePaletteChoiceCatalog("palette", new RecordingLock());
    catalog->addChoice(first, 1);
    ScenePaletteChoiceSelection selection(catalog, 0);

    assert(selection.replacePaletteEntry(0, replacement, 1) != 0);
    PaletteEntry* afterReplace = selection.currentPaletteEntry();
    assert(std::strcmp(afterReplace->Desc(), "Warmer Palette") == 0);
    assert(afterReplace->colors().component(1, 0) == 7);

    int appendedIndex = selection.appendPaletteEntry(appended, 1);
    assert(appendedIndex == 1);
    assert(selection.entryCount() == 2);
    selection.setValue(appendedIndex);
    PaletteEntry* selected = selection.currentPaletteEntry();
    assert(std::strcmp(selected->Name(), "random.1") == 0);
    assert(selected->colors().component(1, 2) == 13);
}

static void testImageCatalogCopiesImageAndPalette() {
    ColorPalette* palette = new ColorPalette();
    palette->setColor(3, 11, 22, 33);
    IndexedImage source("logo", 2, 2, palette);
    source.mutablePixels()[0] = 7;
    source.mutablePixels()[1] = 8;
    source.mutablePixels()[2] = 9;
    source.mutablePixels()[3] = 10;

    SceneImageChoiceCatalog catalog("image", new RecordingLock());
    SceneImageChoice& choice = catalog.addChoice("logo", &source, 1);
    source.mutablePixels()[1] = 99;

    assert(std::strcmp(catalog.optionName(), "image") == 0);
    assert(catalog.entryCount() == 1);
    assert(catalog.choiceAt(0)->sameName("LOGO trailing") != 0);

    const IndexedImage* copied = choice.image();
    assert(copied != &source);
    assert(std::strcmp(copied->name(), "logo") == 0);
    assert(copied->width() == 2);
    assert(copied->height() == 2);
    assert(copied->pixels() != source.pixels());
    assert(copied->pixels()[1] == 8);
    assert(copied->palette() != source.palette());
    assert(copied->palette()->component(3, 0) == 11);
    assert(copied->palette()->component(3, 1) == 22);
    assert(copied->palette()->component(3, 2) == 33);
}

static void testImageSelectionReturnsOwnedImage() {
    IndexedImage first("logo", 1, 1);
    first.mutablePixels()[0] = 5;
    IndexedImage second("badge", 1, 1);
    second.mutablePixels()[0] = 6;
    SceneImageChoiceCatalog* catalog
        = new SceneImageChoiceCatalog("image", new RecordingLock());
    catalog->addChoice("none", 0, 0);
    catalog->addChoice("logo", &first, 1);
    catalog->addChoice("badge", &second, 1);
    SceneImageChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    assert(selection.currentImage() == 0);
    selection.change("badge", randomSource);
    const IndexedImage* selected = selection.currentImage();
    assert(selection.currentValue() == 2);
    assert(selected != &second);
    assert(std::strcmp(selected->name(), "badge") == 0);
    assert(selected->pixels()[0] == 6);
}

static void testWaveObjectChoiceCatalogBuildsFromNativeCatalog() {
    WObject source[] = {
        { { 10, 11, 12 }, { 13, 14, 15 } },
        { { -1, -1, -1 }, { -1, -1, -1 } }
    };
    SceneWaveObjectCatalog waveObjects;
    waveObjects.addChoice("ship", source, 0);
    SceneChoiceCatalog* catalog = createSceneWaveObjectChoiceCatalog(
        "object", new RecordingLock(), waveObjects);

    source[0][0][0] = 99;

    assert(std::strcmp(catalog->optionName(), "object") == 0);
    assert(catalog->entryCount() == 1);
    assert(catalog->choiceAt(0)->sameName("SHIP trailing") != 0);
    assert(catalog->choiceAt(0)->inUse() == 0);

    SceneWaveObjectChoice* choice
        = dynamic_cast<SceneWaveObjectChoice*>(catalog->choiceAt(0));
    assert(choice != 0);
    assert(choice->object()[0][0][0] == 10);
    assert(choice->object()[0][1][2] == 15);

    delete catalog;
}

static void testTranslationChoiceCatalogBuildsFromNativeCatalog() {
    SceneTranslationCatalog translations;
    SceneChoiceCatalog* catalog = createSceneTranslationChoiceCatalog(
        "translate", new RecordingLock(), translations);

    assert(std::strcmp(catalog->optionName(), "translate") == 0);
    assert(catalog->entryCount() == translations.entryCount());
    assert(catalog->choiceAt(0)->sameName("none") != 0);
    assert(catalog->choiceAt(0)->inUse() == translations.inUseAt(0));

    SceneTranslationChoice* choice
        = dynamic_cast<SceneTranslationChoice*>(catalog->choiceAt(0));
    assert(choice != 0);
    assert(std::strcmp(choice->table().name(), "none") == 0);

    delete catalog;
}

static void testPaletteChoiceCatalogBuildsFromNativeCatalog() {
    PaletteEntry source("warm", "Warm Palette");
    source.colors().setColor(8, 21, 22, 23);
    ScenePaletteCatalog palettes;
    palettes.addChoice(source, 0);
    SceneChoiceCatalog* catalog = createScenePaletteChoiceCatalog(
        "palette", new RecordingLock(), palettes);

    source.colors().setColor(8, 90, 90, 90);

    assert(std::strcmp(catalog->optionName(), "palette") == 0);
    assert(catalog->entryCount() == palettes.entryCount());
    assert(catalog->choiceAt(0)->sameName("warm") != 0);
    assert(catalog->choiceAt(0)->inUse() == 0);

    ScenePaletteChoice* choice
        = dynamic_cast<ScenePaletteChoice*>(catalog->choiceAt(0));
    assert(choice != 0);
    assert(choice->paletteEntry()->colors().component(8, 0) == 21);
    assert(choice->paletteEntry()->colors().component(8, 1) == 22);
    assert(choice->paletteEntry()->colors().component(8, 2) == 23);

    delete catalog;
}

static void testImageChoiceCatalogBuildsFromNativeCatalog() {
    IndexedImage image("logo", 1, 1);
    image.mutablePixels()[0] = 42;
    SceneImageCatalog images;
    images.addChoice("logo", &image, 0);
    SceneChoiceCatalog* catalog = createSceneImageChoiceCatalog(
        "image", new RecordingLock(), images);

    image.mutablePixels()[0] = 7;

    assert(std::strcmp(catalog->optionName(), "image") == 0);
    assert(catalog->entryCount() == images.entryCount());
    assert(catalog->choiceAt(0)->sameName("LOGO trailing") != 0);
    assert(catalog->choiceAt(0)->inUse() == 0);

    SceneImageChoice* choice
        = dynamic_cast<SceneImageChoice*>(catalog->choiceAt(0));
    assert(choice != 0);
    assert(choice->image() != &image);
    assert(choice->image()->pixels()[0] == 42);

    delete catalog;
}

int main() {
    testFlameCatalogOwnsTypedChoices();
    testFlameSelectionReturnsTypedFlame();
    testWaveCatalogOwnsTypedChoices();
    testWaveSelectionReturnsTypedWave();
    testWaveObjectCatalogCopiesObjectData();
    testWaveObjectSelectionReturnsOwnedObject();
    testTranslationCatalogCopiesTableData();
    testTranslationSelectionReturnsOwnedTable();
    testPaletteCatalogCopiesEntry();
    testPaletteSelectionReturnsOwnedEntry();
    testPaletteSelectionCanReplaceAndAppendEntries();
    testImageCatalogCopiesImageAndPalette();
    testImageSelectionReturnsOwnedImage();
    testWaveObjectChoiceCatalogBuildsFromNativeCatalog();
    testTranslationChoiceCatalogBuildsFromNativeCatalog();
    testPaletteChoiceCatalogBuildsFromNativeCatalog();
    testImageChoiceCatalogBuildsFromNativeCatalog();
    return 0;
}
