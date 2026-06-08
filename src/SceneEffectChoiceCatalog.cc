// EffectChoice-backed Scene choice catalog adapters.

#include "SceneEffectChoiceCatalog.h"

#include "EffectControl.h"

SceneEffectChoice::SceneEffectChoice(EffectChoice& choice_)
    : choice(choice_) { }

EffectChoice& SceneEffectChoice::effectChoice() {
    return choice;
}

const EffectChoice& SceneEffectChoice::effectChoice() const {
    return choice;
}

const char* SceneEffectChoice::name() const {
    return choice.Name();
}

int SceneEffectChoice::sameName(const char* other) const {
    return const_cast<EffectChoice&>(choice).sameName(other);
}

int SceneEffectChoice::inUse() const {
    return choice.inUse();
}

void SceneEffectChoice::setUse(int inUse_) {
    choice.setUse(inUse_);
}

SceneEffectChoiceLock::SceneEffectChoiceLock(OptionOnOff& lockValue_)
    : lockValue(lockValue_) { }

int SceneEffectChoiceLock::enabled() const {
    return int(lockValue);
}

void SceneEffectChoiceLock::change(const char* to) {
    lockValue.change(to);
}

SceneEffectChoiceCatalog::SceneEffectChoiceCatalog(const char* optionName_,
    EffectChoiceList& entries_, OptionOnOff& lockValue_)
    : optionNameValue(optionName_)
    , entries(entries_)
    , lockValue(lockValue_)
    , choices() { }

int SceneEffectChoiceCatalog::entryCount() const {
    return entries.n();
}

SceneChoice* SceneEffectChoiceCatalog::choiceAt(int index) const {
    EffectChoice* choice = entries[index];
    if (choice == 0)
        return 0;

    while (int(choices.size()) <= index)
        choices.push_back(std::unique_ptr<SceneEffectChoice>());

    if (choices[index].get() == 0
        || &choices[index]->effectChoice() != choice)
        choices[index].reset(new SceneEffectChoice(*choice));

    return choices[index].get();
}

SceneChoiceLock& SceneEffectChoiceCatalog::lock() {
    return lockValue;
}

const SceneChoiceLock& SceneEffectChoiceCatalog::lock() const {
    return lockValue;
}

const char* SceneEffectChoiceCatalog::optionName() const {
    return optionNameValue;
}
