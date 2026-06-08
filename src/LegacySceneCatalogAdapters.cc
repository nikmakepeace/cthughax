// Legacy global Scene catalog adapters.

#include "LegacySceneCatalogAdapters.h"

#include "display.h"

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
};

}

ScenePaletteRandomizer::~ScenePaletteRandomizer() { }

std::unique_ptr<ScenePaletteRandomizer> createLegacyScenePaletteRandomizer() {
    return std::unique_ptr<ScenePaletteRandomizer>(
        new LegacyScenePaletteRandomizer());
}
