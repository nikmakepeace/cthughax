# Visual Benchmark Fixtures

`visual_effects_bench` looks here by default for optional indexed-image
fixtures. The benchmark also has synthetic `zero`, `impulse`, `gradient`, and
`noise` fixtures, so this directory can be empty.

The default benchmark buffer is `1600x1200`. Use
`--cth-buffer-size=WIDTHxHEIGHT` to benchmark another size, for example
`640x480`.

Supported formats:

- `name.pgm`: binary PGM (`P5`), width equal to the benchmark buffer width,
  height equal to the benchmark buffer height or height plus six hidden rows.
- `name.raw`: raw 8-bit indexed bytes, size `width * height` or
  `width * (height + 6)`.

For `--cth-buffer-size=640x480`, valid image sizes are `640x480` and
`640x486`. The taller form includes the three hidden border rows above and
below the visible image. The shorter form populates only the visible image and
leaves hidden rows zeroed.

Optional paired fixtures:

- `name-destination.pgm` / `name-destination.raw`
- `name-source.pgm` / `name-source.raw`

For flame benchmarks, `name-source` is the previous finished image and
`name-destination` is the initial writable buffer. If no pair exists, `name.pgm`
is used as source and destination starts as a copy of source, matching the
framework's copied-source flame contract.

For translation benchmarks, `name.pgm` is used as source and destination starts
at zero unless paired files are present.

Select fixtures with:

```sh
CTH_VISUAL_FIXTURES=gradient,captured build/tests/benchmarks/visual_effects_bench
```

Override the fixture directory with:

```sh
CTH_VISUAL_FIXTURE_DIR=/path/to/fixtures build/tests/benchmarks/visual_effects_bench
```

Use explicit source/destination images with strict size checking:

```sh
build/tests/benchmarks/visual_effects_bench \
  --cth-buffer-size=640x480 \
  --cth-source=/path/to/source.pgm \
  --cth-destination=/path/to/destination.pgm
```

If either explicit image does not match the requested buffer size, the benchmark
exits before registering benchmarks.
