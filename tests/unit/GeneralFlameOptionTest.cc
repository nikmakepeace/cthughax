/** @file
 * Unit coverage for injected general-flame randomness.
 */

#include "ProcessServices.h"
#include "flames.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FixedRandomSource : public RandomSource {
    int value;
    int requestedRangeValue;

public:
    explicit FixedRandomSource(int value_)
        : value(value_)
        , requestedRangeValue(0) { }

    virtual int uniformInt(int exclusiveMax) {
        requestedRangeValue = exclusiveMax;
        if (exclusiveMax <= 1)
            return 0;
        return value % exclusiveMax;
    }

    int requestedRange() const {
        return requestedRangeValue;
    }
};

static void testGeneralFlameRandomUsesInjectedRandomSource() {
    FixedRandomSource randomSource(1234);

    flameGeneral.lock.setValue(0);
    flameGeneral.changeRandom(randomSource, 0);

    assert(randomSource.requestedRange() == 9 * 9 * 9 * 9 * 9);
    assert(int(flameGeneral) == 1234);
}

static void testInvalidGeneralFlameTextUsesInjectedRandomSource() {
    FixedRandomSource randomSource(4321);

    flameGeneral.lock.setValue(0);
    flameGeneral.change("not-a-general-flame", randomSource, 0);

    assert(randomSource.requestedRange() == 9 * 9 * 9 * 9 * 9);
    assert(int(flameGeneral) == 4321);
}

int main() {
    testGeneralFlameRandomUsesInjectedRandomSource();
    testInvalidGeneralFlameTextUsesInjectedRandomSource();
    return 0;
}
