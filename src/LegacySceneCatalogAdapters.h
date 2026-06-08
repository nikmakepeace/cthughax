// Factory functions for legacy global Scene catalog adapters.

#ifndef CTHUGHA_LEGACY_SCENE_CATALOG_ADAPTERS_H
#define CTHUGHA_LEGACY_SCENE_CATALOG_ADAPTERS_H

#include <memory>

class RandomSource;
class PaletteEntry;

class ScenePaletteRandomizer {
public:
    virtual ~ScenePaletteRandomizer();
    virtual int randomizeLast(RandomSource& randomSource) = 0;
    virtual int addRandom(RandomSource& randomSource) = 0;
    virtual PaletteEntry* paletteEntry(int index) = 0;
};

std::unique_ptr<ScenePaletteRandomizer> createLegacyScenePaletteRandomizer();

#endif
