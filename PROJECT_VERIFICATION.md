# Verification Notes

This file records the checks used for the current cleanup and audio-backend
removal work. It is intentionally current-state focused; old generated
architecture notes have been reduced or updated so repository searches point at
live code paths.

## Source Cleanup Checks

The current source/build files no longer contain references to removed audio
backend classes, deleted backend headers, helper-decoder paths, stream-only file
playback options, or old runtime context enums.

The remaining audio model is:

```text
PcmSource -> AudioInput -> AudioBuffer -> AudioOutput
                         -> AudioFrameBuilder -> AudioFrame
```

for file playback, and:

```text
PcmSource -> AudioInput -> AudioInputProcessor -> AudioFrame
```

for live/random input.

Raw file playback is handled by `RawPcmSource` using `sound-format`,
`sound-channels`, and `sound-sample-rate`. WAV and MP3 file playback use the
same runtime path.

## Build Checks

These commands were run successfully after the source cleanup:

```sh
git diff --check
cmake -S /workspaces/cthughanix -B /workspaces/cthughanix/build -DCMAKE_BUILD_TYPE=Debug
cmake --build /workspaces/cthughanix/build --parallel
make -j2
```

The CMake build produced the X11 visualizer and helper tools. The autotools
build also completed, with only pre-existing compiler warnings observed.

## Documentation Sweep

The root project notes now describe the modern audio path, raw PCM support, and
single `AudioFrame` facade. Older generated files under `project-docs/` have
been collapsed into pointers to the root notes so they do not preserve stale
source names or removed extension instructions.

## Remaining Risk Areas

- Live input still depends on OSS-compatible `/dev/dsp` support.
- Mixer and CD controls still use old Unix/Linux ioctls.
- Visual effects still rely heavily on global indexed-buffer state.
- Translation-table loading and compressed indexed-image loading can still spawn helper
  commands.
