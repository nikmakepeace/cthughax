/** @file
 * Unit coverage for WaveRuntime injected randomness.
 */

#include "ProcessServices.h"
#include "Wave.h"
#include "FrameGeneratorContext.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

void wave_dotHor(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_dotVert(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineHor(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineVert(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spike(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spikeH(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff9(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff10(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff11(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff14(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff15(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff16(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete0(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete1(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete2(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_fract1(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_fract2(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_test(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_aaron(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot5(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot55(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot6(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire2(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire2dot1(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineHLdiff(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spiral(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pyro(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_warp(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_laser(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_corner(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_jump(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_sticks(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_grid(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_none(FrameStageBuffer&, const FrameGeneratorContext&, WaveRuntime&) { }

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
};

class NullLogSink : public LogSink {
public:
    virtual int enabled(int) const {
        return 0;
    }

protected:
    virtual void write(int, const char*, int, const char*, va_list) { }
};

static void testWaveRuntimeRandomUsesInjectedSource() {
    std::vector<int> values;
    values.push_back(7);
    values.push_back(3);
    values.push_back(0x800000);
    SequenceRandomSource randomSource(values);
    WaveConfig config;
    WaveState state;
    WaveLookupTables lookupTables;
    NullLogSink log;
    WaveRuntime runtime(config, 0, state, lookupTables, randomSource, log, 0);

    assert(runtime.randomInt(10) == 7);
    assert(runtime.randomCenteredInt(4) == -1);
    double unit = runtime.randomUnit();
    assert(unit > 0.49);
    assert(unit < 0.51);

    assert(randomSource.requestedRange(0) == 10);
    assert(randomSource.requestedRange(1) == 9);
    assert(randomSource.requestedRange(2) == 0x1000000);
}

int main() {
    testWaveRuntimeRandomUsesInjectedSource();
    return 0;
}
