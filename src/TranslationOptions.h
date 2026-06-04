// Translation option/catalog layer.

#ifndef __TRANSLATION_OPTIONS_H__
#define __TRANSLATION_OPTIONS_H__

#include "cthugha.h"
#include "EffectControl.h"
#include "TranslationTable.h"

#include <vector>

class CthughaBuffer;
struct EffectPolicy;

/**
 * Option entry that owns one generated translation table.
 */
class TranslateEntry : public EffectChoice {
    std::vector<int> tableData;
    int widthValue;
    int heightValue;

public:
    /**
     * Creates an entry without mapping data.
     *
     * @param name Option id.
     * @param desc Human-readable description.
     */
    TranslateEntry(const char* name, const char* desc)
        : EffectChoice(name, desc)
        , tableData()
        , widthValue(0)
        , heightValue(0) { }

    /**
     * Creates an entry that owns generated mapping data.
     *
     * @param name Option id.
     * @param desc Human-readable description.
     * @param table Source-pixel indexes, length width * height.
     * @param width Table width in pixels.
     * @param height Table height in pixels.
     */
    TranslateEntry(const char* name, const char* desc,
        const std::vector<int>& table, int width, int height)
        : EffectChoice(name, desc)
        , tableData(table)
        , widthValue(width)
        , heightValue(height) { }

    virtual ~TranslateEntry() { }

    /** @return Borrowed immutable view over this entry's mapping data. */
    TranslationTable table() const {
        return TranslationTable(Name(),
            tableData.empty() ? 0 : &tableData[0],
            widthValue, heightValue);
    }

    friend class TranslateOption;
};

class TranslateOption : public EffectControl {

public:
    TranslateOption(int buffer, const char* name);

    /**
     * @param index Option entry index.
     * @return TranslateEntry at index, or NULL when out of range.
     */
    TranslateEntry* translateEntry(int index);

    /** @return Currently selected translation entry, or NULL. */
    TranslateEntry* currentTranslateEntry();

    /**
     * @param index Option entry index.
     * @return TranslationTable view for index, or an empty table.
     */
    TranslationTable translationTable(int index);

    /** @return TranslationTable view for the current option selection. */
    TranslationTable currentTranslationTable();
};

/**
 * Generates and registers translation tables for a buffer size.
 *
 * @param buffer Cthugha buffer whose width/height define table dimensions.
 * @return Zero on completion.
 */
int init_translate(const CthughaBuffer& buffer);

/** Enables built-in translation table generation/loading. */
extern OptionOnOff use_translates;
void configureTranslationOptions(const EffectPolicy& config);

/** Runtime-selected translation option. */
extern TranslateOption translation;

#endif
