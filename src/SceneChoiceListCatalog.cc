// Owned Scene choice catalog.

#include "SceneChoiceListCatalog.h"

#include <cctype>

namespace {

static int sameChoiceName(const std::string& name, const char* other) {
    if (other == 0)
        return 0;

    const char* left = name.c_str();
    const char* right = other;
    while ((*right != '\0') && (*right != ' ') && (*left != '\0')) {
        if (std::toupper(static_cast<unsigned char>(*right))
            != std::toupper(static_cast<unsigned char>(*left)))
            return 0;
        right++;
        left++;
    }

    return (*left == '\0') && ((*right == '\0') || (*right == ' '));
}

}

SceneChoiceListEntry::SceneChoiceListEntry(const char* name_, int inUse_)
    : nameValue((name_ != 0) ? name_ : "")
    , aliases()
    , inUseValue(inUse_) { }

void SceneChoiceListEntry::addAlias(const char* alias) {
    if (alias != 0)
        aliases.push_back(alias);
}

const char* SceneChoiceListEntry::name() const {
    return nameValue.c_str();
}

int SceneChoiceListEntry::sameName(const char* other) const {
    if (sameChoiceName(nameValue, other))
        return 1;

    for (std::vector<std::string>::const_iterator it = aliases.begin();
         it != aliases.end(); ++it) {
        if (sameChoiceName(*it, other))
            return 1;
    }

    return 0;
}

int SceneChoiceListEntry::inUse() const {
    return inUseValue;
}

void SceneChoiceListEntry::setUse(int inUse_) {
    inUseValue = inUse_;
}

SceneChoiceListCatalog::SceneChoiceListCatalog(
    const char* optionName_, SceneChoiceLock* lock_)
    : optionNameValue((optionName_ != 0) ? optionName_ : "")
    , lockValue(lock_)
    , choices() { }

SceneChoiceListEntry& SceneChoiceListCatalog::addChoice(
    const char* name, int inUse) {
    choices.push_back(std::unique_ptr<SceneChoiceListEntry>(
        new SceneChoiceListEntry(name, inUse)));
    return *choices.back();
}

int SceneChoiceListCatalog::entryCount() const {
    return int(choices.size());
}

SceneChoice* SceneChoiceListCatalog::choiceAt(int index) const {
    if ((index < 0) || (index >= int(choices.size())))
        return 0;
    return choices[index].get();
}

SceneChoiceLock& SceneChoiceListCatalog::lock() {
    return *lockValue;
}

const SceneChoiceLock& SceneChoiceListCatalog::lock() const {
    return *lockValue;
}

const char* SceneChoiceListCatalog::optionName() const {
    return optionNameValue.c_str();
}
