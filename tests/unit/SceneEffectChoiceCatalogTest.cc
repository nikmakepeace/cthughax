#include "SceneEffectChoiceCatalog.h"

#include "EffectControl.h"

#include <cassert>
#include <cstring>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

static void testCatalogAdaptsEffectChoices() {
    EffectChoice first("alpha", "Alpha");
    EffectChoice second("beta", "Beta", 0);
    EffectChoice* entries[] = { &first, &second };
    EffectChoiceList choices(entries, 2);
    OptionOnOff lock("catalog-lock", 0);
    SceneEffectChoiceCatalog catalog("visual", choices, lock);

    assert(std::strcmp(catalog.optionName(), "visual") == 0);
    assert(catalog.entryCount() == 2);
    assert(catalog.choiceAt(-1) == 0);
    assert(catalog.choiceAt(2) == 0);

    SceneChoice* firstChoice = catalog.choiceAt(0);
    SceneChoice* secondChoice = catalog.choiceAt(1);
    assert(firstChoice != 0);
    assert(secondChoice != 0);
    assert(std::strcmp(firstChoice->name(), "alpha") == 0);
    assert(firstChoice->sameName("alpha") != 0);
    assert(secondChoice->inUse() == 0);

    secondChoice->setUse(1);
    assert(second.inUse() == 1);
}

static void testCatalogAdaptsLock() {
    EffectChoice first("alpha", "Alpha");
    EffectChoiceList choices(&first);
    OptionOnOff lock("catalog-lock", 0);
    SceneEffectChoiceCatalog catalog("visual", choices, lock);

    assert(catalog.lock().enabled() == 0);
    catalog.lock().change("on");
    assert(int(lock) == 1);
    assert(catalog.lock().enabled() == 1);
}

static void testChoiceSelectionExposesSceneOwnedToggles() {
    EffectChoice first("alpha", "Alpha");
    EffectChoice second("beta", "Beta");
    EffectChoice* entries[] = { &first, &second };
    EffectChoiceList choices(entries, 2);
    OptionOnOff lock("catalog-lock", 0);
    SceneChoiceSelection selection(
        new SceneEffectChoiceCatalog("visual", choices, lock), 0);

    selection.activate(1);
    assert(selection.currentValue() == 1);
    assert(std::strcmp(selection.currentName(), "beta") == 0);

    selection.toggleChoiceUse(1);
    assert(second.inUse() == 0);

    selection.toggleLock();
    assert(int(lock) == 1);
    selection.toggleLock();
    assert(int(lock) == 0);
}

int main() {
    testCatalogAdaptsEffectChoices();
    testCatalogAdaptsLock();
    testChoiceSelectionExposesSceneOwnedToggles();
    return 0;
}
