#include "SceneChoiceListCatalog.h"

#include "ProcessServices.h"

#include <cassert>
#include <cstring>

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

static void testCatalogOwnsChoicesAndAliases() {
    SceneChoiceListCatalog catalog("simple", new RecordingLock());
    SceneChoiceListEntry& off = catalog.addChoice("off", 1);
    off.addAlias("no");
    off.addAlias("0");
    SceneChoiceListEntry& on = catalog.addChoice("on", 0);
    on.addAlias("yes");
    on.addAlias("1");

    assert(std::strcmp(catalog.optionName(), "simple") == 0);
    assert(catalog.entryCount() == 2);
    assert(catalog.choiceAt(-1) == 0);
    assert(catalog.choiceAt(2) == 0);

    SceneChoice* first = catalog.choiceAt(0);
    SceneChoice* second = catalog.choiceAt(1);
    assert(first != 0);
    assert(second != 0);
    assert(first->sameName("off") != 0);
    assert(first->sameName("no") != 0);
    assert(first->sameName("0") != 0);
    assert(second->sameName("yes") != 0);
    assert(second->sameName("1") != 0);
    assert(second->inUse() == 0);

    second->setUse(1);
    assert(second->inUse() == 1);
}

static void testChoiceSelectionUsesOwnedCatalog() {
    SceneChoiceListCatalog* catalog
        = new SceneChoiceListCatalog("simple", new RecordingLock());
    catalog->addChoice("off", 1);
    catalog->addChoice("on", 1).addAlias("yes");
    SceneChoiceSelection selection(catalog, 0);
    FixedRandomSource randomSource;

    selection.change("yes", randomSource);
    assert(selection.currentValue() == 1);
    assert(std::strcmp(selection.currentName(), "on") == 0);

    selection.toggleChoiceUse(1);
    assert(catalog->choiceAt(1)->inUse() == 0);

    selection.toggleLock();
    assert(catalog->lock().enabled() == 1);
}

int main() {
    testCatalogOwnsChoicesAndAliases();
    testChoiceSelectionUsesOwnedCatalog();
    return 0;
}
