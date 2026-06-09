#include "cthugha.h"
#include "keys.h"

struct KeyNameEntry {
    const char* name;
    int keyValue;
};

static const KeyNameEntry keyNameTable[] = {
    { "F10", CK_FKT(10) },
    { "F11", CK_FKT(11) },
    { "F12", CK_FKT(12) },
    { "F13", CK_FKT(13) },
    { "F14", CK_FKT(14) },
    { "F15", CK_FKT(15) },
    { "F16", CK_FKT(16) },
    { "F17", CK_FKT(17) },
    { "F18", CK_FKT(18) },
    { "F19", CK_FKT(19) },
    { "F20", CK_FKT(20) },
    { "F21", CK_FKT(21) },
    { "F22", CK_FKT(22) },
    { "F23", CK_FKT(23) },
    { "F24", CK_FKT(24) },
    { "F1", CK_FKT(1) },
    { "F2", CK_FKT(2) },
    { "F3", CK_FKT(3) },
    { "F4", CK_FKT(4) },
    { "F5", CK_FKT(5) },
    { "F6", CK_FKT(6) },
    { "F7", CK_FKT(7) },
    { "F8", CK_FKT(8) },
    { "F9", CK_FKT(9) },

    { "Return", CK_ENTER },
    { "Up", CK_UP },
    { "Down", CK_DOWN },
    { "Left", CK_LEFT },
    { "Right", CK_RIGHT },
    { "Prior", CK_PGUP },
    { "Next", CK_PGDN },
    { "End", CK_END },
    { "Home", CK_HOME },
    { "Print", CK_PRINT },
    { "BackSpace", CK_BACK },
    { "Delete", CK_DELETE },

    { "KP_0", '0' },
    { "KP_1", '1' },
    { "KP_2", '2' },
    { "KP_3", '3' },
    { "KP_4", '4' },
    { "KP_5", '5' },
    { "KP_6", '6' },
    { "KP_7", '7' },
    { "KP_8", '8' },
    { "KP_9", '9' },

    { "S-0", CK_SHIFT(0) }, // keymap-only shifted number names
    { "S-1", CK_SHIFT(1) },
    { "S-2", CK_SHIFT(2) },
    { "S-3", CK_SHIFT(3) },
    { "S-4", CK_SHIFT(4) },
    { "S-5", CK_SHIFT(5) },
    { "S-6", CK_SHIFT(6) },
    { "S-7", CK_SHIFT(7) },
    { "S-8", CK_SHIFT(8) },
    { "S-9", CK_SHIFT(9) },
};

static const int keyNameTableCount = sizeof(keyNameTable) / sizeof(KeyNameEntry);

int keyCodeForName(const char* name) {
    if (name == NULL)
        return CK_NONE;

    for (int i = 0; i < keyNameTableCount; i++) {
        if (strcasecmp(name, keyNameTable[i].name) == 0)
            return keyNameTable[i].keyValue;
    }

    return CK_NONE;
}

int keyCodeForNamePrefix(const char* text, int* consumedLength) {
    if (consumedLength != NULL)
        *consumedLength = 0;
    if (text == NULL)
        return CK_NONE;

    for (int i = 0; i < keyNameTableCount; i++) {
        int length = strlen(keyNameTable[i].name);
        if (strncasecmp(keyNameTable[i].name, text, length) == 0) {
            if (consumedLength != NULL)
                *consumedLength = length;
            return keyNameTable[i].keyValue;
        }
    }

    return CK_NONE;
}
