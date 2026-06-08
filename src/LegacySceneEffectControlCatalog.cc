// Legacy EffectControl routing adapter for SceneRuntime.

#include "LegacySceneEffectControlCatalog.h"

#include "EffectControl.h"
#include "LegacySceneEffectControlBindings.h"

namespace {

class LegacySceneEffectControlCatalog : public SceneEffectControlCatalog {
    SceneVisualSelections& selections;
    LegacySceneEffectControlBindings* bindings;

    unsigned int imageChangeFrom(int previousImageValue) {
        return (selections.images().currentValue() != previousImageValue)
            ? SceneImageChanged
            : SceneNoChange;
    }

    unsigned int changeForSelection(
        SceneOptionSelection* selection, int previousImageValue) {
        if (selection == &selections.images())
            return imageChangeFrom(previousImageValue);
        return SceneNoChange;
    }

    SceneOptionSelection* selectionFor(
        const EffectControl& option) const {
        if (bindings == 0)
            return 0;

        return const_cast<SceneOptionSelection*>(
            bindings->selectionFor(option));
    }

public:
    explicit LegacySceneEffectControlCatalog(SceneVisualSelections& selections_)
        : selections(selections_)
        , bindings(legacySceneEffectControlBindings(selections_)) { }

    virtual unsigned int syncFromControls() {
        int previousImageValue = selections.images().currentValue();

        if (bindings != 0)
            bindings->syncFromControls();

        return imageChangeFrom(previousImageValue);
    }

    virtual int isSceneOption(const EffectControl& option) const {
        return selectionFor(option) != 0;
    }

    virtual unsigned int change(
        EffectControl& option, int by, RandomSource& randomSource) {
        (void)randomSource;
        int previousImageValue = selections.images().currentValue();
        SceneOptionSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->change(by);
        return changeForSelection(selection, previousImageValue);
    }

    virtual unsigned int change(EffectControl& option, const char* to,
        RandomSource& randomSource) {
        int previousImageValue = selections.images().currentValue();
        SceneOptionSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->change(to, randomSource);
        return changeForSelection(selection, previousImageValue);
    }

    virtual unsigned int activate(EffectControl& option, int index) {
        int previousImageValue = selections.images().currentValue();
        SceneOptionSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->activate(index);
        return changeForSelection(selection, previousImageValue);
    }

    virtual void toggleLock(EffectControl& option) {
        SceneOptionSelection* selection = selectionFor(option);
        if (selection != 0)
            selection->toggleLock();
    }

    virtual void toggleChoiceUse(EffectControl& option, int index) {
        SceneOptionSelection* selection = selectionFor(option);
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
