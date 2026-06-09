#include "SceneVisualSelectionSet.h"

#include <assert.h>

class StubOptionSelection : public virtual SceneOptionSelection {
public:
    virtual const char* currentName() const { return "stub"; }
    virtual int currentValue() const { return 0; }
    virtual int entryCount() const { return 1; }
    virtual void change(int) { }
    virtual void change(const char*, RandomSource&) { }
    virtual int changeRandom(RandomSource&) { return 0; }
    virtual void setValue(int) { }
};

class StubFlameSelection : public StubOptionSelection,
    public SceneFlameSelection {
public:
    virtual const Flame* currentFlame() { return 0; }
};

class StubGeneralFlameSelection : public StubOptionSelection,
    public SceneGeneralFlameSelection {
public:
    virtual int encodedValue() const { return 0; }
    virtual const char* selectionText() const { return "stub"; }
};

class StubWaveSelection : public StubOptionSelection,
    public SceneWaveSelection {
public:
    virtual Wave* currentWave() { return 0; }
};

class StubTranslationSelection : public StubOptionSelection,
    public SceneTranslationSelection {
public:
    virtual TranslationTable currentTranslationTable() {
        return TranslationTable();
    }
};

class StubPaletteSelection : public StubOptionSelection,
    public ScenePaletteSelection {
public:
    virtual PaletteEntry* currentPaletteEntry() { return 0; }
};

class StubImageSelection : public StubOptionSelection,
    public SceneImageSelection {
public:
    virtual const IndexedImage* currentImage() { return 0; }
};

static void testReturnsOwnedSelectionPorts() {
    StubFlameSelection* flame = new StubFlameSelection();
    StubGeneralFlameSelection* generalFlame
        = new StubGeneralFlameSelection();
    StubWaveSelection* wave = new StubWaveSelection();
    StubOptionSelection* waveScale = new StubOptionSelection();
    StubOptionSelection* table = new StubOptionSelection();
    StubOptionSelection* object = new StubOptionSelection();
    StubTranslationSelection* translation = new StubTranslationSelection();
    StubPaletteSelection* palette = new StubPaletteSelection();
    StubOptionSelection* border = new StubOptionSelection();
    StubOptionSelection* flashlight = new StubOptionSelection();
    StubImageSelection* images = new StubImageSelection();

    SceneVisualSelectionSet selections(flame, generalFlame, wave, waveScale,
        table, object, translation, palette, border, flashlight, images);

    assert(&selections.flame() == flame);
    assert(&selections.generalFlame() == generalFlame);
    assert(&selections.wave() == wave);
    assert(&selections.waveScale() == waveScale);
    assert(&selections.table() == table);
    assert(&selections.object() == object);
    assert(&selections.translation() == translation);
    assert(&selections.palette() == palette);
    assert(&selections.border() == border);
    assert(&selections.flashlight() == flashlight);
    assert(&selections.images() == images);
}

int main() {
    testReturnsOwnedSelectionPorts();
    return 0;
}
