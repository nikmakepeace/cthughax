// -*-c++-*-
//
//    CTHUGHA-L							translate.h
//

#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__

#include "cthugha.h"
#include "CoreOption.h"
#include "TranslationTable.h"

#include <vector>

class CthughaBuffer;

class TranslateEntry : public CoreOptionEntry {
    std::vector<int> tableData;
    int widthValue;
    int heightValue;

public:
    TranslateEntry(const char* name, const char* desc)
        : CoreOptionEntry(name, desc)
        , tableData()
        , widthValue(0)
        , heightValue(0) { }

    TranslateEntry(const char* name, const char* desc,
        const std::vector<int>& table, int width, int height)
        : CoreOptionEntry(name, desc)
        , tableData(table)
        , widthValue(width)
        , heightValue(height) { }

    virtual ~TranslateEntry() { }

    TranslationTable table() const {
        return TranslationTable(Name(),
            tableData.empty() ? 0 : &tableData[0],
            widthValue, heightValue);
    }

    friend class TranslateOption;
};

class TranslateOption : public CoreOption {

public:
    TranslateOption(int buffer, const char* name);

    TranslateEntry* translateEntry(int index);
    TranslateEntry* currentTranslateEntry();
    TranslationTable translationTable(int index);
    TranslationTable currentTranslationTable();
};

int init_translate(const CthughaBuffer& buffer);

extern OptionOnOff use_translates; /* allow translations */
extern TranslateOption translation;

#endif
