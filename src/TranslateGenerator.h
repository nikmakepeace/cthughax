// Built-in translation table generators.

#ifndef __TRANSLATE_GENERATOR_H
#define __TRANSLATE_GENERATOR_H

#include <string>
#include <vector>

class TranslateGenerationTarget {
public:
    int width;
    int height;
    int size;

    TranslateGenerationTarget(int width_, int height_)
        : width(width_)
        , height(height_)
        , size(width_ * height_) { }
};

class TranslateGeneratorOptions {
public:
    int speed;
    int randomness;
    int reverse;
    int slowX;
    int slowY;
    int spiralCount;
    int yinYang;
    double yyWidth;
    double deltaR;
    double deltaA;
    unsigned int seed;
    int randomizeSeed;

    TranslateGeneratorOptions();
};

class GeneratedTranslationTable {
public:
    std::string id;
    std::string description;
    std::vector<int> table;
    int width;
    int height;

    GeneratedTranslationTable();
};

class TranslateGenerator {
public:
    virtual ~TranslateGenerator() { }
    virtual void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const = 0;
};

class TranslationCatalog {
    class Entry {
    public:
        std::string id;
        std::string description;
        const TranslateGenerator* generator;
        TranslateGeneratorOptions options;

        Entry(const char* id_, const char* description_,
            const TranslateGenerator& generator_,
            const TranslateGeneratorOptions& options_);
    };

    std::vector<Entry> entries;

public:
    void add(const char* id, const char* description,
        const TranslateGenerator& generator,
        const TranslateGeneratorOptions& options = TranslateGeneratorOptions());

    void generateAll(int width, int height,
        std::vector<GeneratedTranslationTable>& tables) const;
};

const TranslationCatalog& defaultTranslationCatalog();

#endif
