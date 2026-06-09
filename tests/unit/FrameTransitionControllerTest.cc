#include "FrameTransitionController.h"

#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }

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

static void testPaletteSmoothingUsesExplicitFrameBudget() {
    FrameTransitionController controller;
    controller.configureTransitions(1.0, 3);
    FixedRandomSource random(0);

    assert(controller.paletteSmoothingFrameBudget(24) == 72);
    assert(controller.paletteChangeFrameBudget(random, 24) == 72);
}

static void testPaletteSnapUsesConfiguredProbability() {
    FrameTransitionController controller;
    controller.configureTransitions(0.0, 5);
    FixedRandomSource random(0);

    assert(controller.paletteChangeFrameBudget(random, 30) == 0);
}

static void testQuietMessageDurationUsesExplicitThreshold() {
    FrameTransitionController controller;
    controller.configureQuietMessages(1500);

    assert(controller.quietMessageFrameBudget(60, 2500) == 60);
    assert(controller.quietMessageFrameBudget(30, 1000) == 30);
}

int main() {
    testPaletteSmoothingUsesExplicitFrameBudget();
    testPaletteSnapUsesConfiguredProbability();
    testQuietMessageDurationUsesExplicitThreshold();
    return 0;
}
