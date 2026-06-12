// Wave catalog entries, runtime facade, and shared lookup tables.

#include "Wave.h"
#include "ProcessServices.h"
#include "FrameGeneratorContext.h"

#include <math.h>

void wave_dotHor(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_dotVert(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_lineHor(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_lineVert(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_spike(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_spikeH(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff9(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff10(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff11(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff14(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff15(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_buff16(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_pete0(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_pete1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_pete2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_fract1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_fract2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_test(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_aaron(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire1dot5(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire1dot55(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire1dot6(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_wire2dot1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_lineHLdiff(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_spiral(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_pyro(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_warp(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_laser(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_corner(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_jump(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_sticks(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_grid(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);
void wave_none(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime);

static int waveCanAlwaysRun(const WaveConfig& config) {
    (void)config;

    return 1;
}

static int waveCanRunWithObject(const WaveConfig& config) {
    return config.object != 0;
}

static int waveCanRunAaron(const WaveConfig& config) {
    return config.bufferHeight > 128 && config.bufferWidth >= 256;
}

WaveConfig::WaveConfig()
    : waveScale(0)
    , table(0)
    , object(0)
    , bufferWidth(0)
    , bufferHeight(0) { }

WaveConfig::WaveConfig(int waveScale_, int table_, WObject* object_,
    int bufferWidth_, int bufferHeight_)
    : waveScale(waveScale_)
    , table(table_)
    , object(object_)
    , bufferWidth(bufferWidth_)
    , bufferHeight(bufferHeight_) { }

int WaveConfig::sameAs(const WaveConfig& other) const {
    return waveScale == other.waveScale
        && table == other.table
        && object == other.object
        && bufferWidth == other.bufferWidth
        && bufferHeight == other.bufferHeight;
}

WaveLookupTables::WaveLookupTables()
    : sineWidth(0) { }

const int* WaveLookupTables::sineForWidth(int width) {
    if (width <= 0)
        return 0;

    if (sineWidth != width) {
        static const double fullCircle = 6.28318530717958647692;

        sineWidth = width;
        sineValues.resize(sineWidth);
        for (int i = 0; i < sineWidth; i++)
            sineValues[i] = (int)(128 * sin((double)i / (double)sineWidth * fullCircle));
    }

    return &sineValues[0];
}

int WaveLookupTables::legacySine(int index) {
    if (legacySineValues.empty()) {
        legacySineValues.resize(320);
        for (int i = 0; i < 320; i++)
            legacySineValues[i] = (int)(128 * sin((double)i * 0.03927));
    }

    index %= 320;
    if (index < 0)
        index += 320;

    return legacySineValues[index];
}

double WaveLookupTables::sineDegrees(int degrees) {
    if (degreeSineValues.empty()) {
        degreeSineValues.resize(360);
        for (int i = 0; i < 360; i++)
            degreeSineValues[i] = sin((double)i * (2.0 * M_PI / 360.0));
    }

    degrees %= 360;
    if (degrees < 0)
        degrees += 360;

    return degreeSineValues[degrees];
}

double WaveLookupTables::cosineDegrees(int degrees) {
    return sineDegrees(degrees + 90);
}

WaveRuntime::WaveRuntime(const WaveConfig& config, int needsConfiguration_,
    WaveState& state_, WaveLookupTables& lookupTables_, RandomSource& randomSource_,
    LogSink& log, int fireBudget)
    : needsConfigurationValue(needsConfiguration_)
    , stateValue(state_)
    , lookupTables(lookupTables_)
    , randomSource(randomSource_)
    , logValue(log)
    , fireBudgetValue(fireBudget)
    , waveScale(config.waveScale)
    , table(config.table)
    , object(config.object) { }

int WaveRuntime::needsConfiguration() const {
    return needsConfigurationValue;
}

int WaveRuntime::fire() const {
    return fireBudgetValue;
}

void WaveRuntime::consumeFire() {
    fireBudgetValue = 0;
}

void WaveRuntime::scaleFire(int numerator, int denominator) {
    if (denominator == 0)
        fireBudgetValue = 0;
    else
        fireBudgetValue = fireBudgetValue * numerator / denominator;
}

const int* WaveRuntime::sineForWidth(int width) {
    return lookupTables.sineForWidth(width);
}

int WaveRuntime::legacySine(int index) {
    return lookupTables.legacySine(index);
}

double WaveRuntime::sineDegrees(int degrees) const {
    return lookupTables.sineDegrees(degrees);
}

double WaveRuntime::cosineDegrees(int degrees) const {
    return lookupTables.cosineDegrees(degrees);
}

int WaveRuntime::randomInt(int exclusiveMax) {
    return randomSource.uniformInt(exclusiveMax);
}

int WaveRuntime::randomCenteredInt(int magnitude) {
    return randomInt(magnitude * 2 + 1) - magnitude;
}

double WaveRuntime::randomUnit() {
    static const int scale = 0x1000000;
    return double(randomInt(scale)) / double(scale - 1);
}

LogSink& WaveRuntime::log() const {
    return logValue;
}

Wave::Wave(Function function, const char* name, const char* description,
    CanRunFunction canRunFunction)
    : functionValue(function)
    , canRunFunctionValue(canRunFunction != 0 ? canRunFunction : waveCanAlwaysRun)
    , nameValue(name)
    , descriptionValue(description) { }

const char* Wave::name() const {
    return nameValue;
}

const char* Wave::description() const {
    return descriptionValue;
}

int Wave::canRun(const WaveConfig& config) const {
    return (*canRunFunctionValue)(config);
}

void Wave::execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    const WaveConfig& config, int needsConfiguration, WaveState& state,
    WaveLookupTables& lookupTables, RandomSource& randomSource,
    LogSink& log) const {
    if (functionValue != 0) {
        int fireBudget = context.audioAnalysis().fire();
        WaveRuntime runtime(config, needsConfiguration, state,
            lookupTables, randomSource, log, fireBudget);
        (*functionValue)(buffer, context, runtime);
    }
}

Wave waveCatalog[] = {
    Wave(wave_dotHor, "DotHor", "Dots Horizontal"),
    Wave(wave_dotVert, "DotVert", "Dots Vertical"),
    Wave(wave_lineHor, "LineHor", "Lines Horizontal"),
    Wave(wave_lineVert, "LineVert", "Lines Vertical"),
    Wave(wave_spike, "Spike", "Spikes"),
    Wave(wave_spikeH, "SpikeH", "Spikes Hollow"),
    Wave(wave_buff9, "Walking", "Walking"),
    Wave(wave_buff10, "Falling", "Falling"),
    Wave(wave_buff11, "Lissa", "Lissa"),
    Wave(wave_buff14, "LineX", "Line X"),
    Wave(wave_buff15, "Light1", "Lightning 1"),
    Wave(wave_buff16, "Light2", "Lightning 2"),
    Wave(wave_pete0, "Pete0", "FireFlies"),
    Wave(wave_pete1, "Pete1", "Pete"),
    Wave(wave_pete2, "Pete2", "Dot VS sine"),
    Wave(wave_fract1, "Fract1", "Zippy 1"),
    Wave(wave_fract2, "Fract2", "Zippy 2"),
    Wave(wave_test, "Test", "Test"),
    Wave(wave_aaron, "Aaron", "Rings of Fire", waveCanRunAaron),
    Wave(wave_wire1, "Wire1", "Wire frame 1", waveCanRunWithObject),
    Wave(wave_wire1dot5, "Wire1dot5", "Wire frame 1.5", waveCanRunWithObject),
    Wave(wave_wire1dot55, "Wire1dot55", "Wire frame 1.55", waveCanRunWithObject),
    Wave(wave_wire1dot6, "Wire1dot6", "Wire frame 1.6", waveCanRunWithObject),
    Wave(wave_wire2, "Wire2", "Wire frame 2", waveCanRunWithObject),
    Wave(wave_wire2dot1, "Wire2dot1", "Wire frame 2.1", waveCanRunWithObject),
    Wave(wave_lineHLdiff, "LineHLDiff", "Difference Hor."),
    Wave(wave_spiral, "Spiral", "Spirograph"),
    Wave(wave_pyro, "Pyro", "Fire works"),
    Wave(wave_warp, "Warp", "Space warp"),
    Wave(wave_laser, "Laser", "Laser"),
    Wave(wave_corner, "Corner", "Corner"),
    Wave(wave_jump, "Jump", "Jumping points"),
    Wave(wave_sticks, "Sticks", "Random sticks"),
    Wave(wave_grid, "Grid", "Diagnostic grid"),
    Wave(wave_none, "None", "No wave drawing"),
};

const int nWaveCatalogEntries = sizeof(waveCatalog) / sizeof(Wave);

Wave* waveByIndex(int index) {
    if (index < 0 || index >= nWaveCatalogEntries)
        return 0;

    return waveCatalog + index;
}
