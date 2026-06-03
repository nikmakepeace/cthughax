#include "FramePacer.h"

#include <assert.h>
#include <math.h>

class FakeSleeper : public FrameSleeper {
public:
    int calls;
    double lastSleepSeconds;

    FakeSleeper()
        : calls(0)
        , lastSleepSeconds(0.0) {
    }

    virtual void sleepSeconds(double seconds) {
        calls++;
        lastSleepSeconds = seconds;
    }
};

static void assertNear(double actual, double expected) {
    assert(fabs(actual - expected) < 0.000001);
}

static void testSleepsRemainingBudgetAtFrameEnd() {
    FakeSleeper sleeper;
    FramePacer pacer(sleeper);

    FramePacingResult result = pacer.paceFrameEnd(5.000, 5.004, 100);

    assert(sleeper.calls == 1);
    assertNear(sleeper.lastSleepSeconds, 0.006);
    assertNear(result.frameStartSeconds, 5.000);
    assertNear(result.frameEndSeconds, 5.004);
    assertNear(result.targetFrameEndSeconds, 5.010);
    assertNear(result.requestedSleepSeconds, 0.006);
    assert(result.maxFramesPerSecond == 100);
}

static void testDoesNotSleepWhenFrameWorkOverrunsBudget() {
    FakeSleeper sleeper;
    FramePacer pacer(sleeper);

    FramePacingResult result = pacer.paceFrameEnd(5.000, 5.020, 100);

    assert(sleeper.calls == 0);
    assertNear(result.targetFrameEndSeconds, 5.010);
    assertNear(result.requestedSleepSeconds, 0.0);
}

static void testCarriesDeadlineForwardToCompensateForOversleep() {
    FakeSleeper sleeper;
    FramePacer pacer(sleeper);

    FramePacingResult first = pacer.paceFrameEnd(10.000, 10.010, 25);
    FramePacingResult second = pacer.paceFrameEnd(10.045, 10.055, 25);

    assertNear(first.targetFrameEndSeconds, 10.040);
    assertNear(first.requestedSleepSeconds, 0.030);
    assertNear(second.targetFrameEndSeconds, 10.080);
    assertNear(second.requestedSleepSeconds, 0.025);
    assert(sleeper.calls == 2);
}

static void testMaxFpsZeroDisablesPacing() {
    FakeSleeper sleeper;
    FramePacer pacer(sleeper);

    FramePacingResult result = pacer.paceFrameEnd(5.000, 5.004, 0);

    assert(sleeper.calls == 0);
    assertNear(result.targetFrameEndSeconds, 5.004);
    assertNear(result.requestedSleepSeconds, 0.0);
    assert(result.maxFramesPerSecond == 0);
}

static void testNegativeMaxFpsDisablesPacing() {
    FakeSleeper sleeper;
    FramePacer pacer(sleeper);

    FramePacingResult result = pacer.paceFrameEnd(5.000, 5.004, -20);

    assert(sleeper.calls == 0);
    assertNear(result.targetFrameEndSeconds, 5.004);
    assertNear(result.requestedSleepSeconds, 0.0);
    assert(result.maxFramesPerSecond == -20);
}

int main() {
    testSleepsRemainingBudgetAtFrameEnd();
    testDoesNotSleepWhenFrameWorkOverrunsBudget();
    testCarriesDeadlineForwardToCompensateForOversleep();
    testMaxFpsZeroDisablesPacing();
    testNegativeMaxFpsDisablesPacing();
    return 0;
}
