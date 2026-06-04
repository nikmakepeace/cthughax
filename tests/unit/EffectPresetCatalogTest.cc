#include "Configuration.h"
#include "EffectControl.h"
#include "EffectControlPolicy.h"
#include "EffectPresetCatalog.h"

#include <assert.h>
#include <string.h>

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_context(int, const char*, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

int cth_log_errno(int, const char*, ...) {
    return 0;
}

static void testPresetSlotsOwnWholeEffectSnapshots() {
    EffectChoice displayUp("up", "");
    EffectChoice displayDown("down", "");
    EffectChoice displayMirror("mirror", "");
    EffectChoice* displayEntries[] = { &displayUp, &displayDown, &displayMirror };
    EffectChoiceList displayList(displayEntries, 3);
    EffectControl display(-1, "display", displayList);

    EffectChoice paletteRed("red", "");
    EffectChoice paletteBlue("blue", "");
    EffectChoice paletteGreen("green", "");
    EffectChoice* paletteEntries[] = { &paletteRed, &paletteBlue, &paletteGreen };
    EffectChoiceList paletteList(paletteEntries, 3);
    EffectControl palette(-1, "palette", paletteList);

    EffectChoice borderNone("none", "");
    EffectChoice borderFrame("frame", "");
    EffectChoice* borderEntries[] = { &borderNone, &borderFrame };
    EffectChoiceList borderList(borderEntries, 2);
    EffectControl border(0, "border", borderList);

    EffectPresetCatalog catalog;

    display.change("down", 0);
    palette.change("blue", 0);
    catalog.save(3);

    display.change("up", 0);
    palette.change("red", 0);
    catalog.restore(3);
    assert(strcmp(display.currentName(), "down") == 0);
    assert(strcmp(palette.currentName(), "blue") == 0);

    catalog.setValue(4, display, display.optNr("mirror"));
    catalog.setValue(4, palette, palette.optNr("green"));
    catalog.restore(4);
    assert(strcmp(display.currentName(), "mirror") == 0);
    assert(strcmp(palette.currentName(), "green") == 0);

    catalog.restore(5);
    assert(strcmp(display.currentName(), "up") == 0);
    assert(strcmp(palette.currentName(), "red") == 0);

    EffectPolicy policy;
    policy.allowedChoices.push_back(EffectChoicePolicy("display.down", 0));
    policy.allowedChoices.push_back(EffectChoicePolicy("palette.blue", 0));
    policy.allowedChoices.push_back(EffectChoicePolicy("border.0.frame", 0));
    policy.presets.push_back(EffectPresetPolicy(2, "display", "mirror"));
    policy.presets.push_back(EffectPresetPolicy(2, "palette", "green"));

    configureEffectPolicy(policy);
    assert(displayDown.inUse() == 0);
    assert(paletteBlue.inUse() == 0);
    assert(borderFrame.inUse() == 0);

    display.change("up", 0);
    palette.change("red", 0);
    effectPresetCatalog.restore(2);
    assert(strcmp(display.currentName(), "mirror") == 0);
    assert(strcmp(palette.currentName(), "green") == 0);

    EffectPolicy lazyPolicy;
    lazyPolicy.allowedChoices.push_back(EffectChoicePolicy("late.disabled", 0));
    lazyPolicy.presets.push_back(EffectPresetPolicy(6, "late", "extra"));
    configureEffectPolicy(lazyPolicy);

    EffectChoice lateDefault("default", "");
    EffectChoiceList lateList(&lateDefault);
    EffectControl late(-1, "late", lateList);
    EffectChoice lateDisabled("disabled", "");
    EffectChoice lateExtra("extra", "");
    late.add(&lateDisabled);
    late.add(&lateExtra);

    assert(lateDisabled.inUse() == 0);
    late.change("default", 0);
    effectPresetCatalog.restore(6);
    assert(strcmp(late.currentName(), "extra") == 0);
}

int main() {
    testPresetSlotsOwnWholeEffectSnapshots();
    return 0;
}
