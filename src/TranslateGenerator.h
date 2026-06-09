// Built-in translation table generators.

#ifndef __TRANSLATE_GENERATOR_H
#define __TRANSLATE_GENERATOR_H

#include <string>
#include <vector>

class RandomSource;

class TranslateGenerationTarget {
public:
    /** Output table width in pixels. */
    int width;

    /** Output table height in pixels. */
    int height;

    /** Number of destination pixels, width * height. */
    int size;

    /**
     * Creates a generation target for a buffer size.
     *
     * @param width_ Target buffer width in pixels.
     * @param height_ Target buffer height in pixels.
     */
    TranslateGenerationTarget(int width_, int height_)
        : width(width_)
        , height(height_)
        , size(width_ * height_) { }
};

class TranslateGeneratorOptions {
public:
    /** Motion speed scalar used by generators that model movement. */
    int speed;

    /** Random speed variation percentage, usually clamped to 0-100. */
    int speedJitterPercent;

    /** Random source-offset variation percentage, usually clamped to 0-100. */
    int offsetJitterPercent;

    /** Nonzero to reverse generators that support inward/outward direction. */
    int reverse;

    /** Nonzero to damp x-axis rotation near the center. */
    int slowX;

    /** Nonzero to damp y-axis rotation near the center. */
    int slowY;

    /** Number of spiral centers; zero means a centered single spiral. */
    int spiralCount;

    /** Nonzero to use the yin-yang angular perturbation variant. */
    int yinYang;

    /** Yin-yang band width in pixels/radius units. */
    double yyWidth;

    /** Radial delta applied by spiral generators, in pixels per table step. */
    double deltaR;

    /** Angular delta applied by spiral generators, in radians-ish table units. */
    double deltaA;

    /** Deterministic random seed for generators with random centers/jitter. */
    unsigned int seed;

    /** Nonzero to replace seed with a time-derived seed during catalog generation. */
    int randomizeSeed;

    TranslateGeneratorOptions();
};

/**
 * Generated translation table plus catalog metadata.
 */
class GeneratedTranslationTable {
public:
    /** Stable id used by the translate option. */
    std::string id;

    /** Human-readable description shown by option/UI code. */
    std::string description;

    /** Source-pixel indexes, length width * height. */
    std::vector<int> table;

    /** Table width in pixels. */
    int width;

    /** Table height in pixels. */
    int height;

    GeneratedTranslationTable();
};

/**
 * Base class for built-in translation-table generators.
 */
class TranslateGenerator {
public:
    virtual ~TranslateGenerator() { }

    /**
     * Fills a source-index table for a target buffer size.
     *
     * @param target Output dimensions and entry count.
     * @param options Generator-specific motion/randomization options.
     * @param table Output source-pixel indexes; implementations resize it.
     */
    virtual void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const = 0;
};

/**
 * Registry of built-in translation generators and their option presets.
 */
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
    /**
     * Adds one generated table preset to the catalog.
     *
     * @param id Stable option id.
     * @param description Human-readable option description.
     * @param generator Generator implementation to borrow.
     * @param options Preset generation options.
     */
    void add(const char* id, const char* description,
        const TranslateGenerator& generator,
        const TranslateGeneratorOptions& options = TranslateGeneratorOptions());

    /**
     * Generates every catalog entry for a buffer size.
     *
     * @param width Target buffer width in pixels.
     * @param height Target buffer height in pixels.
     * @param randomSource Seed source for entries whose options request
     *        per-run randomization.
     * @param tables Output generated table collection.
     */
    void generateAll(int width, int height, RandomSource& randomSource,
        std::vector<GeneratedTranslationTable>& tables) const;
};

/** @return Process-wide catalog of built-in translation-table presets. */
const TranslationCatalog& defaultTranslationCatalog();

#endif
