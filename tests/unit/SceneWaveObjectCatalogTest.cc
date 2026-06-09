#include "SceneWaveObjectCatalog.h"

#include <assert.h>
#include <string.h>

static WObject fixtureObject[] = {
    { { 1, 2, 3 }, { 4, 5, 6 } },
    { { 7, 8, 9 }, { 10, 11, 12 } },
    { { -1, -1, -1 }, { -1, -1, -1 } }
};

static void testCatalogOwnsCopiedWaveObjects() {
    SceneWaveObjectCatalog catalog;

    catalog.addChoice("fixture", fixtureObject, 1);
    fixtureObject[0][0][0] = 99;

    assert(catalog.entryCount() == 1);
    assert(strcmp(catalog.nameAt(0), "fixture") == 0);
    assert(catalog.inUseAt(0) == 1);

    const WObject* object = catalog.objectAt(0);
    assert(object != 0);
    assert(object[0][0][0] == 1);
    assert(object[0][0][1] == 2);
    assert(object[0][0][2] == 3);
    assert(object[1][1][0] == 10);
    assert(object[1][1][1] == 11);
    assert(object[1][1][2] == 12);
    assert(object[2][0][0] == -1);
    assert(object[2][0][1] == -1);
    assert(object[2][0][2] == -1);
    assert(object[2][1][0] == -1);
    assert(object[2][1][1] == -1);
    assert(object[2][1][2] == -1);
}

static void testOutOfRangeEntriesAreEmpty() {
    SceneWaveObjectCatalog catalog;

    assert(catalog.entryCount() == 0);
    assert(strcmp(catalog.nameAt(-1), "") == 0);
    assert(strcmp(catalog.nameAt(0), "") == 0);
    assert(catalog.objectAt(-1) == 0);
    assert(catalog.objectAt(0) == 0);
    assert(catalog.inUseAt(-1) == 0);
    assert(catalog.inUseAt(0) == 0);
}

int main() {
    testCatalogOwnsCopiedWaveObjects();
    testOutOfRangeEntriesAreEmpty();
    return 0;
}
