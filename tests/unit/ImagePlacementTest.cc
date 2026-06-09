/** @file
 * Unit coverage for injected image placement randomness.
 */

#include "EffectChoiceLoader.h"
#include "Image.h"
#include "ImagePlacement.h"
#include "ProcessServices.h"
#include "pcx.h"
#include "png.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceLoader) {
    return 0;
}

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceContextLoader, void*) {
    return 0;
}

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

class SequenceRandomSource : public RandomSource {
    std::vector<int> values;
    std::vector<int> ranges;
    unsigned int index;

public:
    SequenceRandomSource(int first, int second)
        : values()
        , ranges()
        , index(0) {
        values.push_back(first);
        values.push_back(second);
    }

    virtual int uniformInt(int exclusiveMax) {
        ranges.push_back(exclusiveMax);
        assert(index < values.size());
        if (exclusiveMax <= 1)
            return 0;
        return values[index++] % exclusiveMax;
    }

    int requestedRange(unsigned int i) const {
        return ranges[i];
    }
};

static void testSmallImagePlacementUsesInjectedRandomSource() {
    IndexedImage image("small", 20, 10);
    SequenceRandomSource randomSource(7, 11);
    RandomLegalImagePlacementStrategy strategy;

    ImagePlacement placement = strategy.choose(image, 100, 50, randomSource);

    assert(randomSource.requestedRange(0) == 81);
    assert(randomSource.requestedRange(1) == 41);
    assert(placement.left == 7);
    assert(placement.top == 11);
    assert(placement.sourceX == 0);
    assert(placement.sourceY == 0);
    assert(placement.destinationX == 7);
    assert(placement.destinationY == 11);
    assert(placement.width == 20);
    assert(placement.height == 10);
    assert(placement.visible());
}

static void testOversizedImagePlacementUsesInjectedRandomSource() {
    IndexedImage image("large", 120, 70);
    SequenceRandomSource randomSource(4, 9);
    RandomLegalImagePlacementStrategy strategy;

    ImagePlacement placement = strategy.choose(image, 100, 50, randomSource);

    assert(randomSource.requestedRange(0) == 21);
    assert(randomSource.requestedRange(1) == 21);
    assert(placement.left == -4);
    assert(placement.top == -9);
    assert(placement.sourceX == 4);
    assert(placement.sourceY == 9);
    assert(placement.destinationX == 0);
    assert(placement.destinationY == 0);
    assert(placement.width == 100);
    assert(placement.height == 50);
    assert(placement.visible());
}

int main() {
    testSmallImagePlacementUsesInjectedRandomSource();
    testOversizedImagePlacementUsesInjectedRandomSource();
    return 0;
}
