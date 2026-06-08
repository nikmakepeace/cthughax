// Typed Scene visual choice catalogs.

#include "SceneTypedVisualCatalogs.h"

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

SceneFlameChoice::SceneFlameChoice(
    const Flame* flame_, const char* name_, int inUse_)
    : flameValue(flame_)
    , nameValue((name_ != 0) ? name_ : "")
    , inUseValue(inUse_) { }

const Flame* SceneFlameChoice::flame() const {
    return flameValue;
}

const char* SceneFlameChoice::name() const {
    return nameValue.c_str();
}

int SceneFlameChoice::sameName(const char* other) const {
    return sameChoiceName(nameValue, other);
}

int SceneFlameChoice::inUse() const {
    return inUseValue;
}

void SceneFlameChoice::setUse(int inUse_) {
    inUseValue = inUse_;
}

SceneFlameChoiceCatalog::SceneFlameChoiceCatalog(
    const char* optionName_, SceneChoiceLock* lock_)
    : optionNameValue((optionName_ != 0) ? optionName_ : "")
    , lockValue(lock_)
    , choices() { }

SceneFlameChoice& SceneFlameChoiceCatalog::addChoice(
    const Flame* flame, const char* name, int inUse) {
    choices.push_back(std::unique_ptr<SceneFlameChoice>(
        new SceneFlameChoice(flame, name, inUse)));
    return *choices.back();
}

int SceneFlameChoiceCatalog::entryCount() const {
    return int(choices.size());
}

SceneChoice* SceneFlameChoiceCatalog::choiceAt(int index) const {
    if ((index < 0) || (index >= int(choices.size())))
        return 0;
    return choices[index].get();
}

SceneChoiceLock& SceneFlameChoiceCatalog::lock() {
    return *lockValue;
}

const SceneChoiceLock& SceneFlameChoiceCatalog::lock() const {
    return *lockValue;
}

const char* SceneFlameChoiceCatalog::optionName() const {
    return optionNameValue.c_str();
}

SceneFlameChoiceSelection::SceneFlameChoiceSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : SceneChoiceSelection(catalog, selectedValue) { }

const Flame* SceneFlameChoiceSelection::currentFlame() {
    SceneFlameChoice* choice
        = dynamic_cast<SceneFlameChoice*>(currentChoice());
    return (choice != 0) ? choice->flame() : 0;
}

SceneWaveChoice::SceneWaveChoice(Wave* wave_, const char* name_, int inUse_)
    : waveValue(wave_)
    , nameValue((name_ != 0) ? name_ : "")
    , inUseValue(inUse_) { }

Wave* SceneWaveChoice::wave() const {
    return waveValue;
}

const char* SceneWaveChoice::name() const {
    return nameValue.c_str();
}

int SceneWaveChoice::sameName(const char* other) const {
    return sameChoiceName(nameValue, other);
}

int SceneWaveChoice::inUse() const {
    return inUseValue;
}

void SceneWaveChoice::setUse(int inUse_) {
    inUseValue = inUse_;
}

SceneWaveChoiceCatalog::SceneWaveChoiceCatalog(
    const char* optionName_, SceneChoiceLock* lock_)
    : optionNameValue((optionName_ != 0) ? optionName_ : "")
    , lockValue(lock_)
    , choices() { }

SceneWaveChoice& SceneWaveChoiceCatalog::addChoice(
    Wave* wave, const char* name, int inUse) {
    choices.push_back(std::unique_ptr<SceneWaveChoice>(
        new SceneWaveChoice(wave, name, inUse)));
    return *choices.back();
}

int SceneWaveChoiceCatalog::entryCount() const {
    return int(choices.size());
}

SceneChoice* SceneWaveChoiceCatalog::choiceAt(int index) const {
    if ((index < 0) || (index >= int(choices.size())))
        return 0;
    return choices[index].get();
}

SceneChoiceLock& SceneWaveChoiceCatalog::lock() {
    return *lockValue;
}

const SceneChoiceLock& SceneWaveChoiceCatalog::lock() const {
    return *lockValue;
}

const char* SceneWaveChoiceCatalog::optionName() const {
    return optionNameValue.c_str();
}

SceneWaveChoiceSelection::SceneWaveChoiceSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : SceneChoiceSelection(catalog, selectedValue) { }

Wave* SceneWaveChoiceSelection::currentWave() {
    SceneWaveChoice* choice = dynamic_cast<SceneWaveChoice*>(currentChoice());
    return (choice != 0) ? choice->wave() : 0;
}
