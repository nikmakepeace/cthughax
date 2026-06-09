#include "SceneTranslationCatalog.h"

#include "ProcessServices.h"

#include <assert.h>
#include <string.h>

class FixedRandomSource : public RandomSource {
public:
    virtual int uniformInt(int exclusiveMax) {
        return exclusiveMax > 1 ? 1 : 0;
    }
};

static void testDisabledCatalogContainsOnlyNoneEntry() {
    FixedRandomSource randomSource;
    SceneTranslationCatalog catalog;

    catalog.load(8, 6, 0, randomSource);

    assert(catalog.entryCount() == 1);
    assert(catalog.generatedCount() == 0);
    assert(strcmp(catalog.tableAt(0).name(), "none") == 0);
    assert(catalog.tableAt(0).ready() == 0);
    assert(catalog.inUseAt(0) == 1);
    assert(catalog.inUseAt(-1) == 0);
    assert(catalog.inUseAt(1) == 0);
}

static void testEnabledCatalogOwnsGeneratedTables() {
    FixedRandomSource randomSource;
    SceneTranslationCatalog catalog;

    catalog.load(8, 6, 1, randomSource);

    assert(catalog.entryCount() > 1);
    assert(catalog.generatedCount() == catalog.entryCount() - 1);

    TranslationTable table = catalog.tableAt(1);
    assert(strcmp(table.name(), "bighalfwheel") == 0);
    assert(table.width() == 8);
    assert(table.height() == 6);
    assert(table.size() == 48);
    assert(table.ready());
    assert(strcmp(catalog.descriptionAt(1), "Krusty wheel 1") == 0);
    assert(catalog.inUseAt(1) == 1);
}

int main() {
    testDisabledCatalogContainsOnlyNoneEntry();
    testEnabledCatalogOwnsGeneratedTables();
    return 0;
}
