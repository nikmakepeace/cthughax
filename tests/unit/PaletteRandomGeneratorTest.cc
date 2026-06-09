/** @file
 * Unit coverage for injected random palette generation.
 */

#include "ColorPalette.h"
#include "PaletteRandomGenerator.h"
#include "ProcessServices.h"

#include <assert.h>
#include <vector>

class SequenceRandomSource : public RandomSource {
    std::vector<int> values;
    std::vector<int> ranges;
    unsigned int index;

public:
    explicit SequenceRandomSource(const std::vector<int>& values_)
        : values(values_)
        , ranges()
        , index(0) { }

    virtual int uniformInt(int exclusiveMax) {
        ranges.push_back(exclusiveMax);
        assert(index < values.size());
        if (exclusiveMax <= 1)
            return 0;
        return values[index++] % exclusiveMax;
    }

    int requestedRange(unsigned int i) const {
        assert(i < ranges.size());
        return ranges[i];
    }

    unsigned int requestCount() const {
        return ranges.size();
    }
};

static void testRandomPaletteUsesInjectedRandomSource() {
    std::vector<int> values;
    values.push_back(0); // two control points
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);
    values.push_back(40);
    values.push_back(50);
    values.push_back(60);
    values.push_back(70);
    values.push_back(80);
    values.push_back(90);
    SequenceRandomSource randomSource(values);
    ColorPalette palette;

    generateRandomPalette(palette, randomSource);

    assert(randomSource.requestCount() == 10);
    assert(randomSource.requestedRange(0) == 3);
    for (unsigned int i = 1; i < randomSource.requestCount(); i++)
        assert(randomSource.requestedRange(i) == 256);

    assert(palette.component(0, 0) == 10);
    assert(palette.component(64, 0) == 15);
    assert(palette.component(128, 0) == 20);
    assert(palette.component(192, 0) == 25);
    assert(palette.component(255, 0) == 29);

    assert(palette.component(0, 1) == 40);
    assert(palette.component(128, 1) == 50);
    assert(palette.component(255, 1) == 59);

    assert(palette.component(0, 2) == 70);
    assert(palette.component(128, 2) == 80);
    assert(palette.component(255, 2) == 89);
}

int main() {
    testRandomPaletteUsesInjectedRandomSource();
    return 0;
}
