// Legacy control mirror synchronizer for SceneRuntime.

#include "LegacySceneSelectionSynchronizer.h"

#include "LegacySceneControlMirror.h"

namespace {

class LegacySceneSelectionSynchronizer : public SceneRuntimeControlBridge {
    SceneVisualSelections& selections;
    LegacySceneControlMirror* mirror;
    int syncedImageValue;

    unsigned int imageChangeFrom(int previousImageValue) {
        return (selections.images().currentValue() != previousImageValue)
            ? SceneImageChanged
            : SceneNoChange;
    }

    void syncBoundControlsFromSelections() {
        if (mirror != 0)
            mirror->syncControlsFromSelections();
        syncedImageValue = selections.images().currentValue();
    }

public:
    explicit LegacySceneSelectionSynchronizer(SceneVisualSelections& selections_)
        : selections(selections_)
        , mirror(legacySceneControlMirror(selections_))
        , syncedImageValue(selections_.images().currentValue()) { }

    virtual unsigned int syncControlsFromSelections() {
        int previousImageValue = syncedImageValue;

        syncBoundControlsFromSelections();

        return imageChangeFrom(previousImageValue);
    }
};

}

std::unique_ptr<SceneRuntimeControlBridge> createLegacySceneSelectionSynchronizer(
    SceneVisualSelections& selections) {
    return std::unique_ptr<SceneRuntimeControlBridge>(
        new LegacySceneSelectionSynchronizer(selections));
}
