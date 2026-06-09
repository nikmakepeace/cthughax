/** @file
 * Unit coverage for injected EffectControl randomness.
 */

#include "EffectControl.h"
#include "EffectRegistry.h"
#include "ProcessServices.h"
#include "RuntimeEffectControls.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class SequenceRandomSource : public RandomSource {
    std::vector<int> values;
    std::vector<int> ranges;
    unsigned int index;

public:
    explicit SequenceRandomSource(const std::vector<int>& values_)
        : values(values_)
        , ranges()
        , index(0) { }

    virtual int uniformInt(int exclusiveMax) {
        ranges.push_back(exclusiveMax);
        assert(index < values.size());
        if (exclusiveMax <= 1)
            return 0;
        return values[index++] % exclusiveMax;
    }

    int requestedRange(unsigned int i) const {
        assert(i < ranges.size());
        return ranges[i];
    }

    unsigned int requestCount() const {
        return ranges.size();
    }
};

static EffectControl& newControl(const char* name, int flags = EFFECT_CONTROL_NO_FLAGS) {
    EffectChoiceList* list = new EffectChoiceList();
    list->add(new EffectChoice("zero", "Zero", 1));
    list->add(new EffectChoice("one", "One", 1));
    list->add(new EffectChoice("two", "Two", 1));
    return *new EffectControl(-1, name, *list, flags);
}

static EffectControl& newAutoChangeControl(const char* name) {
    EffectChoiceList* list = new EffectChoiceList();
    list->add(new EffectChoice("first", "First", 1));
    list->add(new EffectChoice("second", "Second", 1));
    return *new EffectControl(-1, name, *list, EFFECT_CONTROL_AUTO_CHANGE);
}

static void testStaticRandomChangesUseInjectedRandomSource() {
    EffectControl& first = newAutoChangeControl("auto-first");
    EffectControl& second = newAutoChangeControl("auto-second");

    std::vector<int> changeOneValues;
    changeOneValues.push_back(0);
    changeOneValues.push_back(1);
    SequenceRandomSource changeOneRandom(changeOneValues);

    EffectControl* changed = EffectControl::changeOne(changeOneRandom);

    assert(changed == &second);
    assert(int(second) == 1);
    assert(changeOneRandom.requestedRange(0) == 2);
    assert(changeOneRandom.requestedRange(1) == 2);

    std::vector<int> changeAllValues;
    changeAllValues.push_back(1);
    changeAllValues.push_back(0);
    SequenceRandomSource changeAllRandom(changeAllValues);

    EffectControl::changeAll(changeAllRandom);

    assert(int(second) == 1);
    assert(int(first) == 0);
    assert(changeAllRandom.requestedRange(0) == 2);
    assert(changeAllRandom.requestedRange(1) == 2);
}

static void testEffectRegistryRandomChangesUseExplicitControls() {
    EffectControl& first = newAutoChangeControl("registry-first");
    EffectControl& omitted = newAutoChangeControl("registry-omitted");
    EffectControl& second = newAutoChangeControl("registry-second");
    EffectRegistry registry;
    registry.registerControl(first);
    registry.registerControl(second);

    std::vector<int> changeAllValues;
    changeAllValues.push_back(1);
    changeAllValues.push_back(1);
    SequenceRandomSource changeAllRandom(changeAllValues);

    registry.changeAll(changeAllRandom);

    assert(int(first) == 1);
    assert(int(omitted) == 0);
    assert(int(second) == 1);
    assert(changeAllRandom.requestedRange(0) == 2);
    assert(changeAllRandom.requestedRange(1) == 2);

    first.change("first", 0);
    second.change("first", 0);

    std::vector<int> changeOneValues;
    changeOneValues.push_back(1);
    changeOneValues.push_back(1);
    SequenceRandomSource changeOneRandom(changeOneValues);

    EffectControl* changed = registry.changeOne(changeOneRandom);

    assert(changed == &second);
    assert(int(first) == 0);
    assert(int(second) == 1);
    assert(changeOneRandom.requestedRange(0) == 2);
    assert(changeOneRandom.requestedRange(1) == 2);
}

static void testEffectRegistryChangeOneSavesOnlyExplicitControls() {
    EffectControl& registered = newAutoChangeControl("registry-save-registered");
    EffectControl& omitted = newAutoChangeControl("registry-save-omitted");
    EffectRegistry registry;
    registry.registerControl(registered);

    std::vector<int> changeOneValues;
    changeOneValues.push_back(1);
    SequenceRandomSource changeOneRandom(changeOneValues);

    EffectControl* changed = registry.changeOne(changeOneRandom);

    assert(changed == &registered);
    assert(int(registered) == 1);

    omitted.change("second", 0);
    EffectControl::restore();

    assert(int(registered) == 0);
    assert(int(omitted) == 1);
}

static void testChangeRandomUsesInjectedRandomSource() {
    EffectControl& control = newControl("random-control");
    std::vector<int> values;
    values.push_back(2);
    SequenceRandomSource randomSource(values);

    control.changeRandom(randomSource, 0);

    assert(randomSource.requestedRange(0) == 3);
    assert(int(control) == 2);
}

static void testEmptyTextUsesInjectedRandomSource() {
    EffectControl& control = newControl("empty-control");
    std::vector<int> values;
    values.push_back(1);
    SequenceRandomSource randomSource(values);

    control.change("", randomSource, 0);

    assert(randomSource.requestedRange(0) == 3);
    assert(int(control) == 1);
}

static void testInvalidTextUsesInjectedRandomSource() {
    EffectControl& control = newControl("invalid-control");
    std::vector<int> values;
    values.push_back(2);
    SequenceRandomSource randomSource(values);

    control.change("not-a-choice", randomSource, 0);

    assert(randomSource.requestedRange(0) == 3);
    assert(int(control) == 2);
}

static void testRuntimeEffectControlsUseInjectedRandomSource() {
    EffectControl& control = newControl("runtime-effect-control");
    std::vector<int> values;
    values.push_back(1);
    SequenceRandomSource randomSource(values);
    DefaultRuntimeEffectControls runtimeControls(randomSource);

    runtimeControls.changeEffectControlTo(control, "not-a-choice");

    assert(randomSource.requestedRange(0) == 3);
    assert(int(control) == 1);
}

static void testOptNrUsesInjectedRandomSourceForFallbacks() {
    EffectControl& control = newControl("optnr-control");
    std::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    SequenceRandomSource randomSource(values);

    assert(control.optNr("", randomSource) == 1);
    assert(control.optNr("missing", randomSource) == 2);
    assert(control.optNr("-1", randomSource) == 2);
    assert(randomSource.requestedRange(0) == 3);
    assert(randomSource.requestedRange(1) == 3);
    assert(randomSource.requestCount() == 2);
}

int main() {
    testStaticRandomChangesUseInjectedRandomSource();
    testEffectRegistryRandomChangesUseExplicitControls();
    testEffectRegistryChangeOneSavesOnlyExplicitControls();
    testChangeRandomUsesInjectedRandomSource();
    testEmptyTextUsesInjectedRandomSource();
    testInvalidTextUsesInjectedRandomSource();
    testRuntimeEffectControlsUseInjectedRandomSource();
    testOptNrUsesInjectedRandomSourceForFallbacks();
    return 0;
}
