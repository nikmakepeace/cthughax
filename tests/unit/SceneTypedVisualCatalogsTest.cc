#include "SceneTypedVisualCatalogs.h"

#include "ProcessServices.h"

#include <cassert>
#include <cstring>
#include <stdint.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

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
    assert(catalog->lock().enabled() == 1);
}

int main() {
    testFlameCatalogOwnsTypedChoices();
    testFlameSelectionReturnsTypedFlame();
    testWaveCatalogOwnsTypedChoices();
    testWaveSelectionReturnsTypedWave();
    return 0;
}
