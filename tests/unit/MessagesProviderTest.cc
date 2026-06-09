/** @file
 * Unit coverage for injected message-provider randomness.
 */

#include "DefaultMessagesProvider.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <string>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FixedRandomSource : public RandomSource {
    int value;

public:
    explicit FixedRandomSource(int value_)
        : value(value_) { }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;
        return value % exclusiveMax;
    }
};

static void testDefaultMessagesUseInjectedRandomSource() {
    DefaultMessagesProvider provider;
    provider.initialize();

    FixedRandomSource randomSource(1);
    std::string message;

    assert(provider.randomMessage(randomSource, message));
    assert(message == "JOLT !");
}

int main() {
    testDefaultMessagesUseInjectedRandomSource();
    return 0;
}
