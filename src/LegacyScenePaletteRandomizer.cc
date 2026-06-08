// Legacy global Scene palette randomizer.

#include "LegacyScenePaletteRandomizer.h"

#include "PaletteOption.h"

namespace {

class LegacyScenePaletteRandomizer : public ScenePaletteRandomizer {
public:
    virtual int randomizeLast(RandomSource& randomSource) {
        PaletteEntry::randomizeLast(randomSource);
        return PaletteEntry::lastRandomPos;
    }

    virtual int addRandom(RandomSource& randomSource) {
        PaletteEntry::addRandom(randomSource);
        return PaletteEntry::lastRandomPos;
    }

    virtual PaletteEntry* paletteEntry(int index) {
        if ((index < 0) || (index >= palette.getNEntries()))
            return 0;
        return dynamic_cast<PaletteEntry*>(palette[index]);
    }
};

}

std::unique_ptr<ScenePaletteRandomizer> createLegacyScenePaletteRandomizer() {
    return std::unique_ptr<ScenePaletteRandomizer>(
        new LegacyScenePaletteRandomizer());
}
