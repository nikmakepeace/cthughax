#include "FrameClock.h"
#include "ProcessServices.h"

#include <assert.h>
#include <math.h>

static void assertNear(double actual, double expected) {
    assert(fabs(actual - expected) < 0.000001);
}

class FakeSecondsClock : public SecondsClock {
public:
    double value;
    mutable int calls;

    explicit FakeSecondsClock(double value_)
        : value(value_)
        , calls(0) {
    }

    virtual double nowSeconds() const {
        calls++;
        return value;
    }
};

static void testBeginFramePublishesNowAndDelta() {
    FakeSecondsClock clockSource(10.0);
    FrameClock clock(clockSource);

    clock.beginFrame();

    assert(clockSource.calls == 1);
    assert(clock.now() == 10.0);
    assert(clock.deltaT() == 0.0);

    clockSource.value = 10.25;
    clock.beginFrame();

    assert(clockSource.calls == 2);
    assert(clock.now() == 10.25);
    assert(clock.deltaT() == 0.25);
}

static void testAccessorsExposeOwnedFrameTiming() {
    FakeSecondsClock clockSource(20.0);
    FrameClock clock(clockSource);

    clock.beginFrame();

    assert(clock.now() == 20.0);
    assert(clock.deltaT() == 0.0);
}

static void testSampleUsesInjectedTimeSourceWithoutAdvancingFrame() {
    FakeSecondsClock clockSource(30.0);
    FrameClock clock(clockSource);

    assert(clock.sample() == 30.0);
    assert(clock.now() == 0.0);
    assert(clock.deltaT() == 0.0);
}

static void testFrameRateReadoutsComeFromCompletedFrameDurations() {
    FakeSecondsClock clockSource(40.0);
    FrameClock clock(clockSource);

    clock.beginFrame();
    assertNear(clock.framesPerSecond(), 0.0);
    assertNear(clock.rollingFramesPerSecond(), 0.0);

    clockSource.value = 40.040;
    clock.beginFrame();
    assertNear(clock.framesPerSecond(), 25.0);
    assertNear(clock.rollingFramesPerSecond(), 25.0);

    clockSource.value = 40.090;
    clock.beginFrame();
    assertNear(clock.framesPerSecond(), 20.0);
    assertNear(clock.rollingFramesPerSecond(), 2.0 / 0.090);
}

int main() {
    testBeginFramePublishesNowAndDelta();
    testAccessorsExposeOwnedFrameTiming();
    testSampleUsesInjectedTimeSourceWithoutAdvancingFrame();
    testFrameRateReadoutsComeFromCompletedFrameDurations();
    return 0;
}
