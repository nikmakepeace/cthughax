#include "SceneBuiltInChoiceCatalogs.h"

#include "EffectChoiceLoader.h"
#include "Flame.h"
#include "Image.h"
#include "ProcessServices.h"
#include "SceneChoiceSelection.h"
#include "SceneTypedVisualCatalogs.h"
#include "Wave.h"
#include "pcx.h"
#include "png.h"

#include <assert.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceLoader) {
    return 0;
}

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceContextLoader, void*) {
    return 0;
}

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

Flame::Flame(Function function, const char* name, const char* description)
    : functionValue(function)
    , nameValue(name)
    , descriptionValue(description) { }

const char* Flame::name() const {
    return nameValue;
}

const char* Flame::description() const {
    return descriptionValue;
}

void Flame::execute(FrameRenderTarget&, const FrameGeneratorContext&,
    int, const FlameLookupTables&) const { }

const Flame flameCatalog[] = {
    Flame(0, "Clear", "Blank the buffer"),
    Flame(0, "Spark", "Spark"),
    Flame(0, "Glow", "Glow"),
};

extern const int nFlameCatalogEntries
    = sizeof(flameCatalog) / sizeof(Flame);

const Flame* flameByIndex(int index) {
    if (index < 0 || index >= nFlameCatalogEntries)
        return 0;

    return flameCatalog + index;
}

static int waveCanAlwaysRun(const WaveConfig&) {
    return 1;
}

Wave::Wave(Function function, const char* name, const char* description,
    CanRunFunction canRunFunction)
    : functionValue(function)
    , canRunFunctionValue(
        canRunFunction != 0 ? canRunFunction : waveCanAlwaysRun)
    , nameValue(name)
    , descriptionValue(description) { }

const char* Wave::name() const {
    return nameValue;
}

const char* Wave::description() const {
    return descriptionValue;
}

int Wave::canRun(const WaveConfig& config) const {
    return (*canRunFunctionValue)(config);
}

void Wave::execute(FrameRenderTarget&, const FrameGeneratorContext&,
    const WaveConfig&, int, WaveState&, WaveLookupTables&, RandomSource&,
    LogSink&) const { }

Wave waveCatalog[] = {
    Wave(0, "DotHor", "Dots Horizontal"),
    Wave(0, "DotVert", "Dots Vertical"),
    Wave(0, "LineHor", "Lines Horizontal"),
    Wave(0, "LineVert", "Lines Vertical"),
    Wave(0, "Spike", "Spikes"),
    Wave(0, "SpikeH", "Spikes Hollow"),
    Wave(0, "Walking", "Walking"),
    Wave(0, "Falling", "Falling"),
    Wave(0, "Lissa", "Lissa"),
    Wave(0, "LineX", "Line X"),
    Wave(0, "Light1", "Lightning 1"),
    Wave(0, "Light2", "Lightning 2"),
    Wave(0, "Pete0", "FireFlies"),
    Wave(0, "Pete1", "Pete"),
    Wave(0, "Pete2", "Dot VS sine"),
    Wave(0, "Fract1", "Zippy 1"),
    Wave(0, "Fract2", "Zippy 2"),
    Wave(0, "Test", "Test"),
    Wave(0, "Aaron", "Rings of Fire"),
    Wave(0, "Wire1", "Wire frame 1"),
    Wave(0, "Wire1dot5", "Wire frame 1.5"),
    Wave(0, "Wire1dot55", "Wire frame 1.55"),
    Wave(0, "Wire1dot6", "Wire frame 1.6"),
    Wave(0, "Wire2", "Wire frame 2"),
    Wave(0, "Wire2dot1", "Wire frame 2.1"),
    Wave(0, "LineHLDiff", "Difference Hor."),
    Wave(0, "Spiral", "Spirograph"),
    Wave(0, "Pyro", "Fire works"),
    Wave(0, "Warp", "Space warp"),
    Wave(0, "Laser", "Laser"),
    Wave(0, "Corner", "Corner"),
    Wave(0, "Jump", "Jumping points"),
    Wave(0, "Sticks", "Random sticks"),
    Wave(0, "Grid", "Diagnostic grid"),
};

extern const int nWaveCatalogEntries = sizeof(waveCatalog) / sizeof(Wave);

Wave* waveByIndex(int index) {
    if (index < 0 || index >= nWaveCatalogEntries)
        return 0;

    return waveCatalog + index;
}

class RecordingLock : public SceneChoiceLock {
public:
    virtual int enabled() const { return 0; }
    virtual void change(const char*) { }
};

static void testFlameCatalogUsesNativeDefaultAvailability() {
    assert(sceneBuiltInFlameChoiceInUse(-1) == 0);
    assert(sceneBuiltInFlameChoiceInUse(0) == 0);
    assert(sceneBuiltInFlameChoiceInUse(1) == 1);
    assert(sceneBuiltInFlameChoiceInUse(18) == 1);
}

static void testWaveCatalogUsesNativeDefaultAvailability() {
    assert(sceneBuiltInWaveChoiceInUse(-1) == 0);
    assert(sceneBuiltInWaveChoiceInUse(0) == 1);
    assert(sceneBuiltInWaveChoiceInUse(32) == 1);
    assert(sceneBuiltInWaveChoiceInUse(33) == 0);
    assert(sceneBuiltInWaveChoiceInUse(34) == 0);
}

static void testFlameCatalogBuildsTypedNativeChoices() {
    SceneChoiceCatalog* catalog = createSceneFlameChoiceCatalog(
        "flame", new RecordingLock());

    assert(strcmp(catalog->optionName(), "flame") == 0);
    assert(catalog->entryCount() == nFlameCatalogEntries);
    assert(catalog->choiceAt(-1) == 0);
    assert(catalog->choiceAt(nFlameCatalogEntries) == 0);
    assert(catalog->choiceAt(0)->sameName("clear"));
    assert(catalog->choiceAt(0)->inUse() == 0);
    assert(catalog->choiceAt(1)->sameName("SPARK trailing"));
    assert(catalog->choiceAt(1)->inUse() == 1);

    SceneFlameChoice* clear
        = dynamic_cast<SceneFlameChoice*>(catalog->choiceAt(0));
    SceneFlameChoice* spark
        = dynamic_cast<SceneFlameChoice*>(catalog->choiceAt(1));
    assert(clear != 0);
    assert(spark != 0);
    assert(clear->flame() == flameByIndex(0));
    assert(spark->flame() == flameByIndex(1));

    delete catalog;
}

static void testWaveCatalogBuildsTypedNativeChoices() {
    SceneChoiceCatalog* catalog = createSceneWaveChoiceCatalog(
        "wave", new RecordingLock());

    assert(strcmp(catalog->optionName(), "wave") == 0);
    assert(catalog->entryCount() == nWaveCatalogEntries);
    assert(catalog->choiceAt(-1) == 0);
    assert(catalog->choiceAt(nWaveCatalogEntries) == 0);
    assert(catalog->choiceAt(0)->sameName("dothor"));
    assert(catalog->choiceAt(0)->inUse() == 1);
    assert(catalog->choiceAt(33)->sameName("GRID trailing"));
    assert(catalog->choiceAt(33)->inUse() == 0);

    SceneWaveChoice* dot
        = dynamic_cast<SceneWaveChoice*>(catalog->choiceAt(0));
    SceneWaveChoice* grid
        = dynamic_cast<SceneWaveChoice*>(catalog->choiceAt(33));
    assert(dot != 0);
    assert(grid != 0);
    assert(dot->wave() == waveByIndex(0));
    assert(grid->wave() == waveByIndex(33));

    delete catalog;
}

static void assertChoice(SceneChoiceCatalog& catalog, int index,
    const char* name) {
    SceneChoice* choice = catalog.choiceAt(index);
    assert(choice != 0);
    assert(strcmp(choice->name(), name) == 0);
    assert(choice->sameName(name));
    assert(choice->inUse() == 1);
}

static void testWaveScaleCatalogUsesNativeFixedChoices() {
    SceneChoiceCatalog* catalog = createSceneWaveScaleChoiceCatalog(
        "wave-scale", new RecordingLock());

    assert(strcmp(catalog->optionName(), "wave-scale") == 0);
    assert(catalog->entryCount() == 3);
    assertChoice(*catalog, 0, "scale0");
    assertChoice(*catalog, 1, "scale1");
    assertChoice(*catalog, 2, "scale2");

    delete catalog;
}

static void testTableCatalogUsesNativeFixedChoices() {
    SceneChoiceCatalog* catalog = createSceneTableChoiceCatalog(
        "table", new RecordingLock());

    assert(catalog->entryCount() == 10);
    assertChoice(*catalog, 0, "table0");
    assertChoice(*catalog, 9, "table9");

    delete catalog;
}

static void testBorderCatalogUsesNativeFixedChoices() {
    SceneChoiceCatalog* catalog = createSceneBorderChoiceCatalog(
        "border", new RecordingLock());

    assert(catalog->entryCount() == 4);
    assertChoice(*catalog, 0, "border0");
    assertChoice(*catalog, 3, "border3");

    delete catalog;
}

static void testFlashlightCatalogUsesNativeAliases() {
    SceneChoiceCatalog* catalog = createSceneFlashlightChoiceCatalog(
        "flashlight", new RecordingLock());

    assert(catalog->entryCount() == 2);
    assertChoice(*catalog, 0, "off");
    assertChoice(*catalog, 1, "on");
    assert(catalog->choiceAt(0)->sameName("no"));
    assert(catalog->choiceAt(0)->sameName("0"));
    assert(catalog->choiceAt(1)->sameName("yes"));
    assert(catalog->choiceAt(1)->sameName("1"));

    delete catalog;
}

int main() {
    testFlameCatalogUsesNativeDefaultAvailability();
    testWaveCatalogUsesNativeDefaultAvailability();
    testFlameCatalogBuildsTypedNativeChoices();
    testWaveCatalogBuildsTypedNativeChoices();
    testWaveScaleCatalogUsesNativeFixedChoices();
    testTableCatalogUsesNativeFixedChoices();
    testBorderCatalogUsesNativeFixedChoices();
    testFlashlightCatalogUsesNativeAliases();
    return 0;
}
