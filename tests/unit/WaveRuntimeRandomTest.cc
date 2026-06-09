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

void wave_dotHor(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_dotVert(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineHor(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineVert(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spike(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spikeH(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff9(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff10(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff11(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff14(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff15(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_buff16(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete0(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete1(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pete2(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_fract1(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_fract2(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_test(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_aaron(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot5(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot55(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire1dot6(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire2(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_wire2dot1(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_lineHLdiff(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_spiral(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_pyro(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_warp(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_laser(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_corner(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_jump(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_sticks(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_grid(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }
void wave_none(FrameRenderTarget&, const FrameGeneratorContext&, WaveRuntime&) { }

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
