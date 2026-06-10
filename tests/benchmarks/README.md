# Benchmark Guide

This guide is the low-friction path for building, running, and reading the
CthughaNix performance benchmarks.

Use the report targets by default. They run the benchmark, archive the raw
data, and write a `summary.md` you can read first.

The raw binaries, such as `build/tests/benchmarks/visual_effects_bench`, only
print Google Benchmark output to the terminal. They do not create `summary.md`
unless you pass Google Benchmark's own `--benchmark_out=...` flags.

## What To Run

Run these in this order when doing performance work:

1. `scene_script_benchmark_report`
   Runs the real SDL3 application with a deterministic scene script. Use this
   first to find the expensive parts of the actual app path.
2. `visual_effects_benchmark_report`
   Runs focused flame and translation microbenchmarks. Use this when the scene
   script points at visual/filter work.
3. `audio_pipeline_benchmark_report`
   Runs focused audio pipeline microbenchmarks. Use this when investigating
   audio processing cost.

## Step 1: Install Benchmark Dependencies

The scene-script benchmark does not need Google Benchmark. The visual-effects
and audio-pipeline benchmarks do.

On macOS with Homebrew:

```sh
brew install google-benchmark
```

On Debian or Ubuntu:

```sh
sudo apt install libbenchmark-dev
```

If CMake cannot find Google Benchmark on macOS, pass the package directory when
you configure:

```sh
cmake -S . -B build -DCTH_BUILD_BENCHMARKS=ON
cmake --build build --target audio_pipeline_bench
cmake --build build --target visual_effects_bench
cmake --build build --target display_screens_bench
```

## Scene Script Macro Benchmark

`scene_script_benchmark_report` runs `cthugha` at `1600x1200` with the perf
scene script in `tests/fixtures/ini/perf/perf.scenescript`, a fixed audio
fixture, trace logging, and autochange locked by default. It summarizes the real
app path, including per-filter timings grouped by scene, filterchain run timing,
audio-frame timing, display timing, and main-loop timing.

Configure an SDL3 build and run the report target:
-Dbenchmark_DIR="$(brew --prefix google-benchmark)/lib/cmake/benchmark"
```

The missing dependency error usually looks like this:

```text
Could not find a package configuration file provided by "benchmark" with
any of the following names:

  benchmarkConfig.cmake
  benchmark-config.cmake
```

## Step 2: Configure The Build

This builds the current development path: SDL3 frontend, miniaudio, no X11, no
Pulse, and Google Benchmark enabled.

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=OFF \
  -DCTH_BUILD_BENCHMARKS=ON
```

If CMake cannot find Google Benchmark on macOS, use:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=OFF \
  -DCTH_BUILD_BENCHMARKS=ON \
  -Dbenchmark_DIR="$(brew --prefix google-benchmark)/lib/cmake/benchmark"
```

If you only want the scene-script benchmark, you can leave
`CTH_BUILD_BENCHMARKS=OFF`; the scene-script benchmark runs the real app and
does not use Google Benchmark.

## Step 3: Run The Report Benchmarks

Run the real application path first:

```sh
cmake --build build --target scene_script_benchmark_report
```

Then run the focused visual benchmark:

```sh
cmake --build build --target visual_effects_benchmark_report
```

Then run the focused audio benchmark:

```sh
cmake --build build --target audio_pipeline_benchmark_report
```

Each command prints the report directory when it finishes, for example:

```text
Visual effects benchmark report: /path/to/cthughanix/build/test-results/visual-effects/2026-06-09T220000Z-abc123456789
```

Open the `summary.md` in that directory.

## Step 4: Find The Results

Report directories are written here:

```text
build/test-results/scene-script/
build/test-results/visual-effects/
build/test-results/audio-pipeline/
```

Each run gets a timestamped directory named like:

```text
2026-06-09T220000Z-abc123456789
2026-06-09T220000Z-abc123456789-dirty
```

The `-dirty` suffix means the working tree had uncommitted changes when the
benchmark ran.

Open these files first:

- `summary.md`
  Human-readable report. Start here.
- `manifest.json`
  Build and run metadata: git commit, dirty state, compiler, build type,
  buffer size, fixture selection, and command line.

Use these files when you need more detail:

- `trace.log`
  Raw combined app trace for scene-script runs.
- `metrics.json`
  Parsed scene-script metrics.
- `google-benchmark.json`
  Raw Google Benchmark output for visual and audio runs.
- `spread-metrics.json`
  Additional repetition spread metrics for visual and audio runs.
- `console.log`
  Terminal output captured from visual and audio runs.

## Step 5: Read The Scene-Script Summary

The scene-script benchmark is the best first report because it exercises the
real app path with deterministic scene changes.

It uses:

- `1600x1200` buffer size by default.
- `tests/fixtures/ini/perf/perf.scenescript`.
- scene files from `tests/fixtures/ini/perf`.
- `tests/fixtures/audio/sine-100-1600-doubling-10s.wav`.
- SDL3 display path.
- `--silent`, so it does not play audio.
- `--lock` by default, so the random/audio-driven autochanger is disabled.

Read these sections in `summary.md`:

- `Scene Events`
  Confirms which `.ini` scenes were applied and at what script step.
- `Slowest Filters By Scene`
  Best place to start. It shows which named filters cost the most inside each
  scripted scene.
- `Filters By Name`
  Shows whether a filter family is generally expensive across the run.
- `Frame Timing`
  Overall frame generation timing.
- `Display Timing`
  SDL3 presentation timing.
- `Main Loop Timing`
  Total loop timing outside the filterchain.
- `Filterchain Run Timing`
  End-to-end filterchain cost.

How to interpret it:

- Lower `Mean ms`, `Median ms`, and `P99 ms` are better.
- If one filter dominates `Slowest Filters By Scene`, optimize or isolate that
  filter first.
- If `Filterchain Run Timing` is high but no individual filter stands out, look
  for overhead around stage setup, buffer movement, or repeated work.
- If `Display Timing` is high, the bottleneck may be presentation rather than
  filter computation.

Runner script commands that also generate `summary.md`:

```sh
tools/run_scene_script_benchmark.py --build-dir build
tools/run_scene_script_benchmark.py --build-dir build --buffer-size 640x480
tools/run_scene_script_benchmark.py --build-dir build --allow-autochange
```

The runner derives its default process timeout from the scene script: largest
`elapsed-ms` plus 10 seconds, with a 30 second minimum. Use `--timeout N` only
when you want to override that.

## Step 6: Read The Visual-Effects Summary

The visual-effects benchmark isolates flame and translation code over
pre-populated indexed buffers. Fixture copying and buffer reset are excluded
from measured timing.

It uses:

- `1600x1200` buffers by default.
- 20 benchmark repetitions.
- 20 ms minimum time per measured repetition.
- microsecond timing units.
- built-in fixtures: `zero`, `impulse`, `gradient`, and `noise`.

Read these sections in `summary.md`:

- `Slowest Cases`
  Top individual flame/translation cases by mean CPU time.
- `Flame`
  Flame entries grouped for comparison.
- `Translate`
  Translation entries grouped for comparison.
- `MPix/s`
  Visible pixels processed per second. Higher is better.
- `GiB/s`
  Approximate memory throughput. Higher is better.
- `CV %`
  Coefficient of variation across repetitions. Lower is more stable.

How to interpret it:

- Lower `Mean us` and `Median us` are better.
- Higher `MPix/s` and `GiB/s` are better.
- Treat high `CV %` as noise. Rerun before trusting a result with double-digit
  variation.
- Compare cases within the same fixture and buffer size.

Runner script commands that also generate `summary.md`:

```sh
tools/run_visual_effects_benchmarks.py --build-dir build
tools/run_visual_effects_benchmarks.py --build-dir build --buffer-size 640x480
tools/run_visual_effects_benchmarks.py --build-dir build --fixtures gradient
```

## Step 7: Read The Audio-Pipeline Summary

The audio-pipeline benchmark isolates the WAV-to-visual-audio processing path.

It uses:

- 40 benchmark repetitions.
- 1.0 second timed warmup.
- 10 ms audio slices for chunked ingestion/output pipeline benchmarks.
- millisecond timing units.

Read these columns in `summary.md`:

- `Mean ms`
  Typical processing cost.
- `Median ms`
  Typical processing cost with less sensitivity to outliers.
- `P99 ms`
  Tail cost. Spikes show up here.
- `Items/s`
  Throughput for benchmarks that publish an item count.
- `CV %`
  Variation across repetitions. Lower is more stable.

How to interpret it:

- Lower `Mean ms`, `Median ms`, and `P99 ms` are better.
- If `Mean ms` and `Median ms` are close, the run is probably stable.
- If `P99 ms` is much higher than the median, investigate occasional spikes.
- If `CV %` is high, rerun before drawing conclusions.

Runner script commands that also generate `summary.md`:

```sh
tools/run_audio_pipeline_benchmarks.py --build-dir build
tools/run_audio_pipeline_benchmarks.py --build-dir build --repetitions 10
```

## Step 8: Compare Two Runs

Before comparing results, check `manifest.json` in both report directories.
These should match unless you intentionally changed them:

- build type
- compiler
- benchmark command
- buffer size
- fixture selection
- benchmark repetitions
- source commit
- dirty state
- machine

Use this rule of thumb:

- Use scene-script results to identify real app hotspots.
- Use visual/audio microbenchmarks to measure focused changes.
- Trust repeated trends more than one-off wins.
- Rerun noisy cases before changing code based on them.

## Raw Binary Mode

These commands are useful for quick checks, but they do not generate
`summary.md`.

Run the visual benchmark directly:

```sh
build/tests/benchmarks/visual_effects_bench
```

Run selected visual fixtures:

```sh
CTH_VISUAL_FIXTURES=gradient,captured build/tests/benchmarks/visual_effects_bench
```

Run one visual family or entry:

```sh
build/tests/benchmarks/visual_effects_bench --benchmark_filter='Flame/u-Fa'
build/tests/benchmarks/visual_effects_bench --benchmark_filter='Translate/random'
```

Run the audio benchmark directly:

```sh
build/tests/benchmarks/audio_pipeline_bench
```

To create a raw Google Benchmark JSON file yourself:

```sh
build/tests/benchmarks/visual_effects_bench \
  --benchmark_out=visual-effects.json \
  --benchmark_out_format=json
```

Prefer the report runners unless you specifically need raw Google Benchmark
behavior.

## Troubleshooting

No `summary.md` appeared:

- You probably ran a raw benchmark binary directly.
- Run `cmake --build build --target visual_effects_benchmark_report`,
  `audio_pipeline_benchmark_report`, or `scene_script_benchmark_report`
  instead.

`visual_effects_bench` or `audio_pipeline_bench` does not exist:

- Reconfigure with `-DCTH_BUILD_BENCHMARKS=ON`.
- Build or run the report target again.

`cthugha` does not exist for the scene-script benchmark:

- Reconfigure with `-DCTH_BUILD_SDL3=ON`.
- Build or run `scene_script_benchmark_report` again.

CMake cannot find Google Benchmark:

- Install `google-benchmark` or `libbenchmark-dev`.
- On macOS, add
  `-Dbenchmark_DIR="$(brew --prefix google-benchmark)/lib/cmake/benchmark"`.

CMake cannot find SDL3:

- Install the SDL3 development package for your system.
- On macOS with Homebrew, that is usually `brew install sdl3`.

Results are noisy:

- Close other CPU-heavy work.
- Compare runs with the same build type, buffer size, fixtures, and machine.
- Rerun and look for repeated trends rather than one-off changes.

## Display Screens

`display_screens_bench` focuses on the classic screen/display renderers that
map a source indexed buffer into the displayed indexed frame. It measures each
`ScreenEntry` directly, without opening X11 or SDL.

The benchmark has built-in synthetic source patterns:

- `zero`
- `gradient`
- `checker`
- `noise`

Run the default 1600x1200 source-buffer suite:

```sh
build/tests/benchmarks/display_screens_bench
```

Run selected patterns:

```sh
build/tests/benchmarks/display_screens_bench --cth-patterns=gradient,noise
```

Run a different source-buffer size:

```sh
build/tests/benchmarks/display_screens_bench --cth-buffer-size=640x480
```

Filter to one screen:

```sh
build/tests/benchmarks/display_screens_bench --benchmark_filter='Screen/Source'
```

Write a timestamped report:

```sh
cmake --build build --target display_screens_benchmark_report
```

The report runner defaults to:

- 20 benchmark repetitions;
- 20 ms minimum time per measured repetition;
- microsecond timing units;
- spread metrics over all measured repetitions;
- `1600x1200` benchmark buffers unless a different size is passed to the
  runner;
- `zero`, `gradient`, and `noise` source patterns.

Run the report runner directly:

```sh
tools/run_display_screens_benchmarks.py --build-dir build
```

Reports are written under:

```text
build/test-results/display-screens/
```

Each report directory contains `summary.md`, `trace.log`,
`google-benchmark.json`, and `spread-metrics.json`.
