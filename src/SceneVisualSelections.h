// Visual selection ports used by Scene visual catalog adapters.

#ifndef CTHUGHA_SCENE_VISUAL_SELECTIONS_H
#define CTHUGHA_SCENE_VISUAL_SELECTIONS_H

#include "Scene.h"

class RandomSource;
class SceneChoice;

/**
 * Common operations for one selectable visual scene option.
 */
class SceneOptionSelection {
public:
    virtual ~SceneOptionSelection();

    virtual const char* catalogName() const;
    virtual const char* currentName() const = 0;
    virtual int currentValue() const = 0;
    virtual int entryCount() const = 0;
    virtual int choiceCount() const;
    virtual SceneChoice* choiceAt(int index);
    virtual const SceneChoice* choiceAt(int index) const;
    virtual int resolveValue(const char* text, int* selection) const;
    virtual void change(int by) = 0;
    virtual void change(
        const char* to, RandomSource& randomSource) = 0;
    virtual int changeRandom(RandomSource& randomSource) = 0;

    /**
     * Activates a choice by index.
     *
     * The default implementation sets the selection value when the index is
     * within entryCount().
     */
    virtual void activate(int index);

    /** @return Nonzero when this selection is locked against random changes. */
    virtual int lockEnabled() const;

    /** Toggles the selection lock when the concrete selection has one. */
    virtual void toggleLock();

    /** Toggles choice availability when choices are exposed. */
    virtual void toggleChoiceUse(int index);

    virtual void setValue(int index) = 0;
};

class SceneFlameSelection : public virtual SceneOptionSelection {
public:
    virtual const Flame* currentFlame() = 0;
};

class SceneGeneralFlameSelection : public virtual SceneOptionSelection {
public:
    virtual int encodedValue() const = 0;
    virtual const char* selectionText() const = 0;
};

class SceneWaveSelection : public virtual SceneOptionSelection {
public:
    virtual Wave* currentWave() = 0;
};

/**
 * Selection that exposes the typed object payload used by object-capable waves.
 */
class SceneWaveObjectSelection : public virtual SceneOptionSelection {
public:
    /** @return Selected 3D object line list, or NULL. */
    virtual WObject* currentObject() = 0;
};

class SceneTranslationSelection : public virtual SceneOptionSelection {
public:
    virtual TranslationTable currentTranslationTable() = 0;
};

class ScenePaletteSelection : public virtual SceneOptionSelection {
public:
    virtual PaletteEntry* currentPaletteEntry() = 0;
};

class SceneImageSelection : public virtual SceneOptionSelection {
public:
    virtual const IndexedImage* currentImage() = 0;
};

/**
 * Explicit collection of visual selections that back a SceneVisualCatalogs.
 */
class SceneVisualSelections {
public:
    virtual ~SceneVisualSelections();

    virtual SceneFlameSelection& flame() = 0;
    virtual SceneGeneralFlameSelection& generalFlame() = 0;
    virtual SceneWaveSelection& wave() = 0;
    virtual SceneOptionSelection& waveScale() = 0;
    virtual SceneOptionSelection& table() = 0;
    virtual SceneOptionSelection& object() = 0;
    virtual SceneTranslationSelection& translation() = 0;
    virtual ScenePaletteSelection& palette() = 0;
    virtual SceneOptionSelection& border() = 0;
    virtual SceneOptionSelection& flashlight() = 0;
    virtual SceneImageSelection& images() = 0;
};

#endif
