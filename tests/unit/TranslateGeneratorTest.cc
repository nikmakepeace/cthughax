/** @file
 * Unit coverage for translation catalog generation dependencies.
 */

#include "ProcessServices.h"
#include "TranslateGenerator.h"

#include <assert.h>
#include <vector>

class SequenceRandomSource : public RandomSource {
    std::vector<int> values;
    unsigned int index;

public:
    SequenceRandomSource()
        : values()
        , index(0) {
        values.push_back(0x1234);
        values.push_back(0xabcd);
    }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;

        assert(index < values.size());
        return values[index++] % exclusiveMax;
    }

    unsigned int calls() const {
        return index;
    }
};

class CapturingTranslateGenerator : public TranslateGenerator {
public:
    mutable std::vector<unsigned int> seeds;

    virtual void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        seeds.push_back(options.seed);
        table.assign(target.size, 0);
    }
};

static void testCatalogRandomizesSeedsFromInjectedRandomSource() {
    TranslationCatalog catalog;
    CapturingTranslateGenerator generator;

    TranslateGeneratorOptions stableOptions;
    stableOptions.seed = 55;
    catalog.add("stable", "Stable", generator, stableOptions);

    TranslateGeneratorOptions randomOptions;
    randomOptions.seed = 77;
    randomOptions.randomizeSeed = 1;
    catalog.add("random", "Random", generator, randomOptions);

    SequenceRandomSource randomSource;
    std::vector<GeneratedTranslationTable> tables;
    catalog.generateAll(2, 2, randomSource, tables);

    assert(randomSource.calls() == 2);
    assert(tables.size() == 2);
    assert(tables[0].table.size() == 4);
    assert(tables[1].table.size() == 4);
    assert(generator.seeds.size() == 2);
    assert(generator.seeds[0] == 55);
    assert(generator.seeds[1] == (0x1234abcdu ^ (0x9e3779b9u * 2u)));
}

int main() {
    testCatalogRandomizesSeedsFromInjectedRandomSource();
    return 0;
}
