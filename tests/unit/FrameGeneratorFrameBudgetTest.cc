#include "FrameGeneratorFrameBudget.h"

#include <assert.h>

static void testConfiguredMaxFramesPerSecondWins() {
    assert(frameGenerationBudgetFramesPerSecond(25, 59.8) == 25);
}

static void testRollingFramesPerSecondFeedsUncappedBudgets() {
    assert(frameGenerationBudgetFramesPerSecond(0, 48.9) == 48);
}

static void testMissingRollingFramesPerSecondFallsBackToSixty() {
    assert(frameGenerationBudgetFramesPerSecond(0, 0.0) == 60);
}

int main() {
    testConfiguredMaxFramesPerSecondWins();
    testRollingFramesPerSecondFeedsUncappedBudgets();
    testMissingRollingFramesPerSecondFallsBackToSixty();
    return 0;
}
