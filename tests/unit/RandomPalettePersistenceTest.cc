/** @file
 * Unit coverage for persisted random palette name allocation.
 */

#include "ProcessServices.h"
#include "PaletteOption.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
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

static void writePlaceholderPalette(const char* path) {
    FILE* file = fopen(path, "wb");
    assert(file != NULL);
    fputs("0 0 0\n", file);
    fclose(file);
}

static void assertFileExists(const char* path) {
    assert(access(path, F_OK) == 0);
}

static void testRandomPaletteNamesContinueAfterPersistedFilesAndLoadedEntries() {
    char originalCwd[PATH_MAX];
    char tempTemplate[] = "/tmp/cthughanix-random-palettes-XXXXXX";
    char* tempDir = mkdtemp(tempTemplate);
    assert(tempDir != NULL);
    assert(getcwd(originalCwd, sizeof(originalCwd)) != NULL);
    assert(chdir(tempDir) == 0);
    assert(mkdir("resources", 0777) == 0);
    assert(mkdir("resources/map", 0777) == 0);

    writePlaceholderPalette("resources/map/random.0.map");
    writePlaceholderPalette("resources/map/random.2.map");

    PaletteEntry::lastRandom = -1;
    PaletteEntry::lastRandomPos = -1;
    CountingRandomSource randomSource;

    PaletteEntry::addRandom(randomSource);
    assert(PaletteEntry::lastRandom == 3);
    assertFileExists("resources/map/random.3.map");

    palette.setValue(PaletteEntry::lastRandomPos);
    PaletteEntry::randomizeLast(randomSource);
    assert(PaletteEntry::lastRandom == 3);
    assertFileExists("resources/map/random.3.map");

    palette.add(new PaletteEntry("random.8", ""));
    PaletteEntry::addRandom(randomSource);
    assert(PaletteEntry::lastRandom == 9);
    assertFileExists("resources/map/random.9.map");

    assert(chdir(originalCwd) == 0);
}

int main() {
    testRandomPaletteNamesContinueAfterPersistedFilesAndLoadedEntries();
    return 0;
}
