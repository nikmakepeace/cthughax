# Benchmarks

These benchmarks use Google Benchmark. They are disabled by default so normal
builds do not require benchmark headers.

Configure and build:

```sh
cmake -S . -B build -DCTH_BUILD_BENCHMARKS=ON
cmake --build build --target audio_pipeline_bench
cmake --build build --target visual_effects_bench
```

## Audio Pipeline

`audio_pipeline_bench` focuses on the WAV-to-visual-audio path.

Run directly:

```sh
build/tests/benchmarks/audio_pipeline_bench
```

Write a timestamped report:

```sh
cmake --build build --target audio_pipeline_benchmark_report
```

The report runner defaults to:

- 40 benchmark repetitions;
- a timed warmup before measured repetitions (default: 1.0 seconds);
- 10 ms audio slices for chunked ingestion/output pipeline benchmarks;
- millisecond timing units;
- spread metrics over all measured repetitions.

Reports are written under:

```text
build/test-results/audio-pipeline/
```

The report directory contains Google Benchmark JSON, console output,
`manifest.json` with git/build/fixture metadata, `spread-metrics.json`, and
`summary.md`.

## Visual Effects

`visual_effects_bench` focuses on flame and translation throughput over
pre-populated indexed buffers. Fixture copying and buffer reset are excluded
from measured timing.

The benchmark has built-in synthetic fixtures:

- `zero`
- `impulse`
- `gradient`
- `noise`

It can also load image fixtures from `tests/fixtures/visual` or from
`CTH_VISUAL_FIXTURE_DIR`. See `tests/fixtures/visual/README.md` for the fixture
contract.

Run the default fixture set:

```sh
build/tests/benchmarks/visual_effects_bench
```

Run selected fixtures:

```sh
CTH_VISUAL_FIXTURES=gradient,captured build/tests/benchmarks/visual_effects_bench
```

Run a larger synthetic fixture:

```sh
build/tests/benchmarks/visual_effects_bench --cth-buffer-size=640x480
```

Run explicit active/passive images with strict size checking:

```sh
build/tests/benchmarks/visual_effects_bench \
  --cth-buffer-size=640x480 \
  --cth-active=/path/to/active.pgm \
  --cth-passive=/path/to/passive.pgm
```

For `640x480`, each explicit image must be either `640x480` or `640x486`.
The taller form includes Cthugha's three hidden border rows above and below the
visible buffer.

Filter to one family or entry:

```sh
build/tests/benchmarks/visual_effects_bench --benchmark_filter='Flame/u-Fa'
build/tests/benchmarks/visual_effects_bench --benchmark_filter='Translate/random'
```

Write a timestamped report:

```sh
cmake --build build --target visual_effects_benchmark_report
```

The report runner defaults to:

- 20 benchmark repetitions;
- 20 ms minimum time per measured repetition;
- microsecond timing units;
- spread metrics over all measured repetitions;
- `160x100` benchmark buffers unless a different size is passed to the runner.

Run a sized report directly:

```sh
tools/run_visual_effects_benchmarks.py --build-dir build --buffer-size 640x480
```

Run a specific fixture at its encoded size:

```sh
tools/run_visual_effects_benchmarks.py \
  --build-dir build \
  --buffer-size 640x480 \
  --fixtures 640x480-fixture
```

Reports are written under:

```text
build/test-results/visual-effects/
```

Each report directory contains Google Benchmark JSON, console output,
`manifest.json` with git/build/fixture metadata, `spread-metrics.json`, and
`summary.md`. The visual `summary.md` includes the requested buffer size, the
selected fixture name, and a fixture-file table showing the decoded image size
and whether it matched the buffer.
