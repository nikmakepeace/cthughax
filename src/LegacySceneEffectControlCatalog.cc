// Legacy EffectControl routing adapter for SceneRuntime.

#include "LegacySceneEffectControlCatalog.h"

#include "EffectControl.h"
#include "LegacySceneEffectControlSelection.h"

namespace {

class LegacySceneEffectControlCatalog : public SceneEffectControlCatalog {
    SceneVisualSelections& selections;

    static SceneEffectControlSelection* effectControlSelection(
        SceneOptionSelection& selection) {
        return dynamic_cast<SceneEffectControlSelection*>(&selection);
    }

    static void syncSelection(SceneOptionSelection& selection) {
        SceneEffectControlSelection* effectSelection
            = effectControlSelection(selection);
        if (effectSelection != 0)
            effectSelection->syncFromControl();
    }

    unsigned int imageChangeFrom(int previousImageValue) {
        return (selections.images().currentValue() != previousImageValue)
            ? SceneImageChanged
            : SceneNoChange;
    }

    unsigned int changeForSelection(
        SceneEffectControlSelection* selection, int previousImageValue) {
        if (selection == effectControlSelection(selections.images()))
            return imageChangeFrom(previousImageValue);
        return SceneNoChange;
    }

    SceneEffectControlSelection* selectionFor(
        const EffectControl& option) const {
        SceneEffectControlSelection* selection;

        selection = effectControlSelection(selections.flame());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.generalFlame());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.wave());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.waveScale());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.object());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.translation());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.border());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.flashlight());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.palette());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.table());
        if (selection != 0 && selection->isOption(option))
            return selection;
        selection = effectControlSelection(selections.images());
        if (selection != 0 && selection->isOption(option))
            return selection;

        return 0;
    }

public:
    explicit LegacySceneEffectControlCatalog(SceneVisualSelections& selections_)
        : selections(selections_) { }

    virtual unsigned int syncFromControls() {
        int previousImageValue = selections.images().currentValue();

        syncSelection(selections.flame());
        syncSelection(selections.generalFlame());
        syncSelection(selections.wave());
        syncSelection(selections.waveScale());
        syncSelection(selections.table());
        syncSelection(selections.object());
        syncSelection(selections.translation());
        syncSelection(selections.palette());
        syncSelection(selections.border());
        syncSelection(selections.flashlight());
        syncSelection(selections.images());

        return imageChangeFrom(previousImageValue);
    }

    virtual int isSceneOption(const EffectControl& option) const {
        return selectionFor(option) != 0;
    }

    virtual unsigned int change(
        EffectControl& option, int by, RandomSource& randomSource) {
        (void)randomSource;
        int previousImageValue = selections.images().currentValue();
        SceneEffectControlSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->change(by);
        return changeForSelection(selection, previousImageValue);
    }

    virtual unsigned int change(EffectControl& option, const char* to,
        RandomSource& randomSource) {
        int previousImageValue = selections.images().currentValue();
        SceneEffectControlSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->change(to, randomSource);
        return changeForSelection(selection, previousImageValue);
    }

    virtual unsigned int activate(EffectControl& option, int index) {
        int previousImageValue = selections.images().currentValue();
        SceneEffectControlSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->activate(index);
        return changeForSelection(selection, previousImageValue);
    }

    virtual void toggleLock(EffectControl& option) {
        SceneEffectControlSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->toggleLock();
    }

    virtual void toggleChoiceUse(EffectControl& option, int index) {
        SceneEffectControlSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->toggleChoiceUse(index);
    }
};

}

std::unique_ptr<SceneEffectControlCatalog> createLegacySceneEffectControlCatalog(
    SceneVisualSelections& selections) {
    return std::unique_ptr<SceneEffectControlCatalog>(
        new LegacySceneEffectControlCatalog(selections));
}
