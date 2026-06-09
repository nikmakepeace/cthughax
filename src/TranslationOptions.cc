// Translation option/catalog setup; generated tables are executed by Translate.

#include "cthugha.h"
#include "Configuration.h"
#include "ProcessServices.h"
#include "SceneTranslationCatalog.h"
#include "SceneGeometry.h"
#include "TranslationOptions.h"
#include "TranslateGenerator.h"

#include <vector>

OptionOnOff use_translates("use-translate", 0); /* allow translations */

static EffectChoice* _trans[] = { new TranslateEntry("none", "No Translate") };
static EffectChoiceList translateEntries(_trans, 1);
TranslateOption translation(-1, "translate");

void configureTranslationOptions(const EffectPolicy& config) {
    use_translates.setValue(config.useTranslatesEnabled);
}

/*
 * Initialize the translate-tables.
 */
int init_translate(const SceneGeometry& geometry, RandomSource& randomSource) {
    SceneTranslationCatalog catalog;
    catalog.load(geometry.width(), geometry.height(), int(use_translates),
        randomSource);
    return init_translate(catalog);
}

int init_translate(const SceneTranslationCatalog& catalog) {
    if (catalog.generatedCount() > 0) {

        CTH_INFO("  loading translation tables...\n");

        for (int i = 1; i < catalog.entryCount(); i++) {
            TranslationTable table = catalog.tableAt(i);
            std::vector<int> tableData;
            if (table.data() != 0 && table.size() > 0)
                tableData.assign(table.data(), table.data() + table.size());
            translation.add(new TranslateEntry(table.name(),
                catalog.descriptionAt(i), tableData, table.width(),
                table.height()));
        }

        CTH_INFO("  number of loaded translates: %d\n",
            translation.getNEntries());
    }

    return 0;
}

TranslateOption::TranslateOption(int buffer, const char* name)
    : EffectControl(buffer, name, translateEntries, EFFECT_CONTROL_AUTO_CHANGE) { }

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
