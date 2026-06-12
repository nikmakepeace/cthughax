// Flame catalog entries and runtime dispatch.

#include "Flame.h"

void flame_clear(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_upslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_upsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_upfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_leftslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_leftsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_leftfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_rightslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_rightsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_rightfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_water(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_watersubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_skyline(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_weird(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_zzz(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_fade(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_general_subtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_general_slow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_down(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime);

FlameRuntime::FlameRuntime(int generalFlame_, const FlameLookupTables& lookupTables_)
    : generalFlame(generalFlame_)
    , lookupTables(lookupTables_) { }

Flame::Flame(Function function, const char* name, const char* description)
    : functionValue(function)
    , nameValue(name)
    , descriptionValue(description) { }

const char* Flame::name() const {
    return nameValue;
}

const char* Flame::description() const {
    return descriptionValue;
}

void Flame::execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    int generalFlame, const FlameLookupTables& lookupTables) const {
    if (functionValue != 0) {
        FlameRuntime runtime(generalFlame, lookupTables);
        (*functionValue)(buffer, context, runtime);
    }
}

const Flame flameCatalog[] = {
    Flame(flame_clear, "Clear", "Blank the buffer"),
    Flame(flame_upslow, "u-Sl", "Up Slow"),
    Flame(flame_upsubtle, "u-Su", "Up Subtle"),
    Flame(flame_upfast, "u-Fa", "Up Fast"),
    Flame(flame_leftslow, "l-Sl", "Left Slow"),
    Flame(flame_leftsubtle, "l-Su", "Left Subtle"),
    Flame(flame_leftfast, "l-Fa", "Left Fast"),
    Flame(flame_rightslow, "r-Sl", "Right Slow"),
    Flame(flame_rightsubtle, "r-Su", "Right Subtle"),
    Flame(flame_rightfast, "r-Fa", "Right Fast"),
    Flame(flame_water, "Water", "Water"),
    Flame(flame_watersubtle, "Wa-s", "Water Subtle"),
    Flame(flame_skyline, "Skyline", "Skyline"),
    Flame(flame_weird, "Weird", "Weird"),
    Flame(flame_zzz, "Zzz", "Zzz"),
    Flame(flame_fade, "Fade", "Fade"),
    Flame(flame_general_subtle, "GenSubt", "General Subtle"),
    Flame(flame_general_slow, "GenSlow", "General Slow"),
    Flame(flame_down, "Down", "Falling Down"),
};

const int nFlameCatalogEntries = sizeof(flameCatalog) / sizeof(Flame);

const Flame* flameByIndex(int index) {
    if (index < 0 || index >= nFlameCatalogEntries)
        return 0;

    return flameCatalog + index;
}
