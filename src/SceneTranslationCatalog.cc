// Native Scene catalog owner for generated translation tables.

#include "SceneTranslationCatalog.h"

#include "TranslateGenerator.h"

SceneTranslationCatalog::Entry::Entry()
    : name("none")
    , description("No Translate")
    , table()
    , width(0)
    , height(0)
    , inUse(1) { }

SceneTranslationCatalog::Entry::Entry(const char* name_,
    const char* description_, const int* data_, int size_, int width_,
    int height_, int inUse_)
    : name((name_ != 0) ? name_ : "none")
    , description((description_ != 0) ? description_ : "")
    , table()
    , width(width_)
    , height(height_)
    , inUse(inUse_) {
    if (data_ != 0 && size_ > 0)
        table.assign(data_, data_ + size_);
}

SceneTranslationCatalog::SceneTranslationCatalog()
    : entries() {
    entries.push_back(Entry());
}

void SceneTranslationCatalog::load(int width, int height,
    int translationsEnabled, RandomSource& randomSource) {
    entries.clear();
    entries.push_back(Entry());

    if (!translationsEnabled)
        return;

    std::vector<GeneratedTranslationTable> generatedTables;
    defaultTranslationCatalog().generateAll(width, height, randomSource,
        generatedTables);
    for (unsigned int i = 0; i < generatedTables.size(); i++) {
        const GeneratedTranslationTable& generated = generatedTables[i];
        entries.push_back(Entry(generated.id.c_str(),
            generated.description.c_str(),
            generated.table.empty() ? 0 : &generated.table[0],
            int(generated.table.size()), generated.width, generated.height, 1));
    }
}

int SceneTranslationCatalog::entryCount() const {
    return int(entries.size());
}

int SceneTranslationCatalog::generatedCount() const {
    return entryCount() > 0 ? entryCount() - 1 : 0;
}

TranslationTable SceneTranslationCatalog::tableAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return TranslationTable();

    const Entry& entry = entries[index];
    return TranslationTable(entry.name.c_str(),
        entry.table.empty() ? 0 : &entry.table[0], entry.width, entry.height);
}

const char* SceneTranslationCatalog::descriptionAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return "";

    return entries[index].description.c_str();
}

int SceneTranslationCatalog::inUseAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index].inUse;
}
