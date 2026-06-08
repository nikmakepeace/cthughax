#include "FrameTransitionController.h"

#include "Configuration.h"
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
    SceneTransitionPolicy policy;
    policy.paletteSmoothingChance = 1.0;
    policy.paletteSmoothSeconds = 3;
    controller.configureTransitions(policy);
    FixedRandomSource random(0);

    assert(controller.paletteSmoothingFrameBudget(24) == 72);
    assert(controller.paletteChangeFrameBudget(random, 24) == 72);
}

static void testPaletteSnapUsesConfiguredProbability() {
    FrameTransitionController controller;
    SceneTransitionPolicy policy;
    policy.paletteSmoothingChance = 0.0;
    policy.paletteSmoothSeconds = 5;
    controller.configureTransitions(policy);
    FixedRandomSource random(0);

    assert(controller.paletteChangeFrameBudget(random, 30) == 0);
}

static void testQuietMessageDurationUsesExplicitThreshold() {
    FrameTransitionController controller;
    MessagesConfig messages;
    messages.quietMessageMs = 2500;
    messages.quietMessageDurationMs = 1500;
    controller.configureQuietMessages(messages);

    assert(controller.quietMessageFrameBudget(60, 2500) == 60);
    assert(controller.quietMessageFrameBudget(30, 1000) == 30);
}

int main() {
    testPaletteSmoothingUsesExplicitFrameBudget();
    testPaletteSnapUsesConfiguredProbability();
    testQuietMessageDurationUsesExplicitThreshold();
    return 0;
}
