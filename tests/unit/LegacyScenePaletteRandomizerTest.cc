/** @file
 * Unit coverage for the legacy palette randomizer adapter boundary.
 */

#include "LegacyScenePaletteRandomizer.h"

#include "PaletteOption.h"
#include "ProcessServices.h"

#include <assert.h>
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
char* cthugha_mode_text() { return (char*)""; }
int cth_init(int*, char**) { return 0; }
int cth_main() { return 0; }

class CountingRandomSource : public RandomSource {
    int value;

public:
    CountingRandomSource()
        : value(0) { }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;

        return value++ % exclusiveMax;
    }
};

static void assertFileExists(const char* path) {
    assert(access(path, F_OK) == 0);
}

static void testRandomizerUsesExplicitCurrentPaletteIndex() {
    char originalCwd[PATH_MAX];
    char tempTemplate[] = "/tmp/cthughanix-legacy-randomizer-XXXXXX";
    char* tempDir = mkdtemp(tempTemplate);
    assert(tempDir != NULL);
    assert(getcwd(originalCwd, sizeof(originalCwd)) != NULL);
    assert(chdir(tempDir) == 0);
    assert(mkdir("resources", 0777) == 0);
    assert(mkdir("resources/map", 0777) == 0);

    palette.add(new PaletteEntry("random.0", ""));
    palette.add(new PaletteEntry("random.1", ""));
    palette.setValue(0);
    PaletteEntry::lastRandom = -1;
    PaletteEntry::lastRandomPos = -1;

    CountingRandomSource randomSource;
    std::unique_ptr<ScenePaletteRandomizer> randomizer
        = createLegacyScenePaletteRandomizer();

    int changedIndex = randomizer->randomizeLast(randomSource, 1);

    assert(changedIndex == 1);
    assert(palette.currentN() == 1);
    assert(PaletteEntry::lastRandom == 1);
    assert(PaletteEntry::lastRandomPos == 1);
    assertFileExists("resources/map/random.1.map");

    assert(chdir(originalCwd) == 0);
}

int main() {
    testRandomizerUsesExplicitCurrentPaletteIndex();
    return 0;
}
