#include "Configuration.h"
#include "InputQueue.h"
#include "keys.h"

#include <assert.h>

static void testRawKeysAreQueuedInOrder() {
    InputQueue queue;

    queue.pushRawKey("a", 0);
    queue.pushRawKey("Return", 0);

    assert(queue.size() == 2);
    assert(queue.popKey() == 'a');
    assert(queue.popKey() == CK_ENTER);
    assert(queue.popKey() == CK_NONE);
}

static void testEscapeKeyFollowsInputConfig() {
    InputQueue queue;
    char escapeText[2] = { 27, 0 };

    queue.pushRawKey(escapeText, 0);
    assert(queue.popKey() == CK_NONE);

    InputConfig config;
    config.escapeKeyEnabled = 1;
    queue.configure(config);
    queue.pushRawKey(escapeText, 0);

    assert(queue.popKey() == CK_ESC);
    assert(queue.popKey() == CK_NONE);
}

static void testSingleCharacterTranslation() {
    InputQueue queue;

    queue.pushRawKey("\r", 0);
    queue.pushRawKey("\b", 0);
    queue.pushRawKey("7", 1);

    assert(queue.popKey() == CK_ENTER);
    assert(queue.popKey() == CK_BACK);
    assert(queue.popKey() == CK_SHIFT(7));
}

static void testNamedKeyTranslationAndUnknownNames() {
    InputQueue queue;

    queue.pushRawKey("F10", 0);
    queue.pushRawKey("DefinitelyUnknown", 0);
    queue.pushRawKey("KP_4", 0);

    assert(queue.popKey() == CK_FKT(10));
    assert(queue.popKey() == '4');
    assert(queue.popKey() == CK_NONE);
}

static void testKeymapPrefixLookupUsesLongestOrderedSymbols() {
    int consumed = 0;
    int key = keyCodeForNamePrefix("F10 restore(0)", &consumed);

    assert(key == CK_FKT(10));
    assert(consumed == 3);

    consumed = 0;
    key = keyCodeForNamePrefix("F1 help()", &consumed);

    assert(key == CK_FKT(1));
    assert(consumed == 2);
}

int main() {
    testRawKeysAreQueuedInOrder();
    testEscapeKeyFollowsInputConfig();
    testSingleCharacterTranslation();
    testNamedKeyTranslationAndUnknownNames();
    testKeymapPrefixLookupUsesLongestOrderedSymbols();
    return 0;
}
