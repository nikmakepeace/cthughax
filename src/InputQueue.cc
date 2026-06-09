#include "InputQueue.h"

#include "Configuration.h"
#include "keys.h"

KeyTranslator::KeyTranslator()
    : escapeKeyEnabled(0) {
}

void KeyTranslator::configure(const InputConfig& config) {
    escapeKeyEnabled = config.escapeKeyEnabled;
}

static int shiftedNumberKey(int key, int shifted) {
    if (!shifted)
        return key;

    if ((key >= '0') && (key <= '9'))
        return CK_SHIFT(key - '0');

    return key;
}

int KeyTranslator::translate(const char* text, int shifted) const {
    if ((text == 0) || (text[0] == '\0'))
        return CK_NONE;

    if (text[1] == '\0') {
        int key = CK_NONE;
        signed char first = text[0];
        switch (first) {
        case 0:
        case -1:
            key = CK_NONE;
            break;
        case 27:
            key = escapeKeyEnabled ? CK_ESC : CK_NONE;
            break;
        case 10:
        case 13:
            key = CK_ENTER;
            break;
        case 8:
            key = CK_BACK;
            break;
        default:
            key = first;
            break;
        }
        return shiftedNumberKey(key, shifted);
    }

    return keyCodeForName(text);
}

void InputQueue::configure(const InputConfig& config) {
    translator.configure(config);
}

void InputQueue::pushRawKey(const char* text, int shifted) {
    int key = translator.translate(text, shifted);
    if (key != CK_NONE)
        pushKey(key);
}

void InputQueue::pushKey(int key) {
    if (key != CK_NONE)
        keys.push_back(key);
}

int InputQueue::popKey() {
    if (keys.empty())
        return CK_NONE;

    int key = keys.front();
    keys.pop_front();
    return key;
}

int InputQueue::empty() const {
    return keys.empty();
}

size_t InputQueue::size() const {
    return keys.size();
}
