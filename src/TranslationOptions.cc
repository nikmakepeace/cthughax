// Translation option/catalog setup; generated tables are executed by Translate.

#include "cthugha.h"
#include "TranslationOptions.h"
#include "TranslateGenerator.h"
#include "CthughaBuffer.h"

#include <vector>

OptionOnOff use_translates("use-translate", DEFAULT_USE_TRANSLATES_ENABLED); /* allow translations */

static CoreOptionEntry* _trans[] = { new TranslateEntry("none", "No Translate") };
static CoreOptionEntryList translateEntries(_trans, 1);
TranslateOption translation(-1, "translate");

/*
 * Initialize the translate-tables.
 */
int init_translate(const CthughaBuffer& buffer) {

    if (use_translates) {

        CTH_INFO("  loading translation tables...\n");

        std::vector<GeneratedTranslationTable> generatedTables;
        defaultTranslationCatalog().generateAll(buffer.width(), buffer.height(), generatedTables);
        for (unsigned int i = 0; i < generatedTables.size(); i++) {
            const GeneratedTranslationTable& generated = generatedTables[i];
            translation.add(new TranslateEntry(generated.id.c_str(), generated.description.c_str(),
                generated.table, generated.width, generated.height));
        }

        CTH_INFO("  number of loaded translates: %d\n",
            translation.getNEntries());
    }

    return 0;
}

TranslateOption::TranslateOption(int buffer, const char* name)
    : CoreOption(buffer, name, translateEntries, CORE_OPTION_AUTO_CHANGE) { }

TranslateEntry* TranslateOption::translateEntry(int index) {
    if ((index < 0) || (index >= getNEntries()))
        return NULL;

    return static_cast<TranslateEntry*>(entries[index]);
}

TranslateEntry* TranslateOption::currentTranslateEntry() {
    return translateEntry(currentN());
}

TranslationTable TranslateOption::translationTable(int index) {
    TranslateEntry* entry = translateEntry(index);
    return (entry != 0) ? entry->table() : TranslationTable();
}

TranslationTable TranslateOption::currentTranslationTable() {
    return translationTable(currentN());
}
