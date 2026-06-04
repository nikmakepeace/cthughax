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
```

The CMake build produced the X11 visualizer and helper tools.

## Display Presentation Checks

`presentation_composer_test`, `cthugha_display_presentation_test`, and
`screen_renderer_characterization_test` cover the screen rejection contract:
renderers may reject incompatible geometry, while `PresentationComposer` owns
frame-local fallback without mutating the selected display option.

## Runtime Command Checks

The runtime reconfiguration seam has focused tests for command construction,
mediator routing including close/save-and-continue and palette metadata
commands, and AutoChanger's command-sink consumer behavior:

```sh
ctest --test-dir build --output-on-failure -R 'runtime_command_test|runtime_change_mediator_test|auto_changer_command_sink_test'
```

The full local verification pass after adding those tests was:

```sh
cmake --build build
tests/headers/check-headers.sh
ctest --test-dir build --output-on-failure
```

The full suite currently reports 32 passing tests.

## Documentation Sweep

The root project notes now describe the modern audio path, raw PCM support,
single `AudioFrame` facade, and CMake-only build.

## Remaining Risk Areas

- Live input still depends on OSS-compatible `/dev/dsp` support.
- Mixer controls still use old Unix/Linux ioctls.
- Visual effects still rely heavily on global indexed-buffer state.
- Compressed indexed-image loading can still spawn helper commands.
