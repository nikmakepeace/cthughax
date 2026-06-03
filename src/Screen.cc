#include "Screen.h"
#include "ScreenRenderContext.h"

int screen_up(ScreenRenderContext&);
int screen_down(ScreenRenderContext&);
int screen_2hor(ScreenRenderContext&);
int screen_r2hor(ScreenRenderContext&);
int screen_4hor(ScreenRenderContext&);
int screen_2verd(ScreenRenderContext&);
int screen_r2verd(ScreenRenderContext&);
int screen_4kal(ScreenRenderContext&);
int screen_hfield(ScreenRenderContext&);
int screen_roll(ScreenRenderContext&);
int screen_zick(ScreenRenderContext&);
int screen_bent(ScreenRenderContext&);
int screen_plate(ScreenRenderContext&);
int screen_vscale_hmirror(ScreenRenderContext&);
int screen_hscale_vmirror(ScreenRenderContext&);
int screen_source(ScreenRenderContext&);

ScreenEntry::ScreenEntry(Function function, const char* name, const char* description,
    xy filledScale, int inUse)
    : ScreenEntry(function, name, description, filledScale, xy(2, 2), inUse) { }

ScreenEntry::ScreenEntry(Function function, const char* name, const char* description,
    xy filledScale, xy outputScale, int inUse)
    : EffectChoice(name, description, inUse)
    , functionValue(function)
    , filledScaleValue(filledScale)
    , outputScaleValue(outputScale) { }

int ScreenEntry::operator()() {
    ScreenRenderContext* context = currentScreenRenderContext();
    return context != 0 ? render(*context) : 0;
}

int ScreenEntry::render(ScreenRenderContext& context) {
    if (functionValue == 0)
        return 0;

    ScopedScreenRenderContext contextScope(context);
    return (*functionValue)(context);
}

xy ScreenEntry::filledScale() const {
    return filledScaleValue;
}

xy ScreenEntry::filledOutputSize(int sourceWidth, int sourceHeight) const {
    return scaledOutputSize(sourceWidth, sourceHeight, filledScaleValue);
}

xy ScreenEntry::outputSize(int sourceWidth, int sourceHeight) const {
    return scaledOutputSize(sourceWidth, sourceHeight, outputScaleValue);
}

ScreenEntry screenCatalog[] = {
    ScreenEntry(screen_up, "Up", "Up Display", xy(1, 1)),               // 0
    ScreenEntry(screen_down, "Down", "Upside Down", xy(1, 1)),          // 1
    ScreenEntry(screen_2hor, "2hor", "Hor. Split out", xy(1, 1)),       // 2
    ScreenEntry(screen_r2hor, "r2hor", "Hor. Split in", xy(1, 1)),      // 3
    ScreenEntry(screen_4hor, "4hor", "Kaleidoscope", xy(1, 1)),         // 4
    ScreenEntry(screen_2verd, "2verd", "90deg rot. mirror", xy(1, 1)),  // 5
    ScreenEntry(screen_r2verd, "r2verd", "90deg rot. mirror II", xy(1, 1)), // 6
    ScreenEntry(screen_4kal, "4kal", "90deg Kaleidoscope", xy(1, 1)),   // 7
    ScreenEntry(screen_hfield, "hfield", "Heightfield", xy(2, 2)),      // 8
    ScreenEntry(screen_roll, "roll", "Roll around x-axis", xy(1, 1)),   // 9
    ScreenEntry(screen_zick, "zick", "Zick Zack", xy(1, 1), 0),         // 10
    ScreenEntry(screen_bent, "bent", "A bending plane", xy(2, 2)),      // 11
    ScreenEntry(screen_plate, "plate", "A rotating plate", xy(2, 2)),   // 12

    ScreenEntry(screen_vscale_hmirror, "scaley", "Scale vertical, mirror horizontal", xy(1, 1), xy(2, 1)), // 13
    ScreenEntry(screen_hscale_vmirror, "scalex", "Scale horizontal, mirror vertical", xy(1, 1), xy(1, 2)), // 14
    ScreenEntry(screen_source, "Source", "Source size", xy(1, 1), xy(1, 1)), // 15
};

const int nScreenCatalogEntries = sizeof(screenCatalog) / sizeof(ScreenEntry);

ScreenEntry* screenByIndex(int index) {
    if (index < 0 || index >= nScreenCatalogEntries)
        return 0;

    return screenCatalog + index;
}

static EffectChoice* screenChoiceEntries[] = {
    screenCatalog + 0,
    screenCatalog + 1,
    screenCatalog + 2,
    screenCatalog + 3,
    screenCatalog + 4,
    screenCatalog + 5,
    screenCatalog + 6,
    screenCatalog + 7,
    screenCatalog + 8,
    screenCatalog + 9,
    screenCatalog + 10,
    screenCatalog + 11,
    screenCatalog + 12,
    screenCatalog + 13,
    screenCatalog + 14,
    screenCatalog + 15,
};

static_assert(sizeof(screenChoiceEntries) / sizeof(EffectChoice*) == nScreenCatalogEntries,
    "screen choice table must match screen catalog");

EffectChoiceList screenEntries(screenChoiceEntries, nScreenCatalogEntries);

EffectControl screen(-1, "display", screenEntries, EFFECT_CONTROL_AUTO_CHANGE);
