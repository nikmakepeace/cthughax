# Build and Porting Notes

## Build System

CMake is the only supported build system.

The active build files are:

```text
CMakeLists.txt
cmake/config.h.in
cmake/generate_keymap.cmake
src/CMakeLists.txt
tests/benchmarks/CMakeLists.txt
```

The old autotools entry points have been removed from the tracked source tree.
Generated configure-era files may still appear in dirty working directories,
but they are ignored and are not part of the build contract.

## CMake Build

The normal build command is:

```sh
cmake -S . -B build
cmake --build build
```

The main executable target is:

- `xcthugha`: the X11 visualizer.

Major CMake options:

- `CTH_BUILD_X11`: build the X11 frontend. Default: `ON`.
- `CTH_BUILD_TESTS`: build and register focused unit tests. Default: `ON`.
- `CTH_BUILD_BENCHMARKS`: build Google Benchmark performance suites. Default:
  `OFF`.
- `CTH_RUN_AUDIO_DEVICE_TESTS`: register opt-in smoke tests that open real
  audio devices. Default: `OFF`.
- `CTH_ENABLE_PULSE`: enable PulseAudio/PipeWire-Pulse output when
  `libpulse-simple` is available. Default: `ON`.
- `CTH_ENABLE_DSP`: enable OSS `/dev/dsp` support when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_MIXER`: enable OSS mixer controls when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_MINIMP3`: enable embedded minimp3 decoding. Default: `ON`.
- `CTH_ENABLE_MINIAUDIO`: enable vendored miniaudio playback and capture
  devices. Default: `ON`.
- `CTH_MINIAUDIO_NO_RUNTIME_LINKING`: on Apple builds, define
  `MA_NO_RUNTIME_LINKING` and link miniaudio's required frameworks explicitly.
  Default: `OFF`.
- `CTH_DATA_DIR`: installed runtime data directory. Default:
  `${CMAKE_INSTALL_FULL_DATADIR}/cthughanix`.

CMake writes the generated configuration header to:

```text
build/config.h
```

Source files should include it as `config.h` through the target include path,
not by reaching for a source-tree generated header.

## Native Dependencies

Core/common:

- C and C++ compiler;
- CMake;
- POSIX process/file APIs.

X11 frontend:

- X11;
- Xt;
- Xaw;
- Xmu;
- Xext;
- Zlib;
- optional Xpm.

Audio and media:

- embedded minimp3 for native MP3 decoding;
- vendored miniaudio under `external/miniaudio` for cross-platform playback
  and capture;
- optional PulseAudio-compatible output via `libpulse-simple`;
- optional OSS `/dev/dsp`;
- optional OSS mixer ioctls.

Miniaudio does not require a system miniaudio package. On Linux it runtime-loads
native backends such as ALSA, PulseAudio, JACK, and sndio when available. The
project disables miniaudio's legacy OSS/audio(4) backends and keeps the
existing Cthugha OSS code as the only OSS path. On Apple, the default build uses
miniaudio runtime framework loading; app-bundle/signing paths can configure
with `CTH_MINIAUDIO_NO_RUNTIME_LINKING=ON` to link CoreFoundation, CoreAudio,
and AudioToolbox explicitly.

## Verified Commands

Useful verification commands:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
env -u DISPLAY build/src/xcthugha --help
```

Useful miniaudio verification commands from a clean build directory:

```sh
cmake -S . -B build-miniaudio-x11 -G Ninja \
  -DCTH_BUILD_X11=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-miniaudio-x11
ctest --test-dir build-miniaudio-x11 --output-on-failure
```

```sh
cmake -S . -B build-miniaudio-no-pulse-oss -G Ninja \
  -DCTH_BUILD_X11=OFF \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_ENABLE_DSP=OFF \
  -DCTH_ENABLE_MIXER=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-miniaudio-no-pulse-oss
ctest --test-dir build-miniaudio-no-pulse-oss --output-on-failure
```

Real-device miniaudio smoke tests are opt-in because they open playback and
capture devices:

```sh
cmake -S . -B build-miniaudio-device-tests -G Ninja \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_RUN_AUDIO_DEVICE_TESTS=ON
cmake --build build-miniaudio-device-tests
ctest --test-dir build-miniaudio-device-tests \
  -R 'miniaudio_.*_smoke' --output-on-failure
```

Those smoke tests intentionally fail if miniaudio falls back to the Null
backend, because passing them is evidence of a real audio device path.

When the default device is not the target device, pass exact miniaudio device
names at runtime:

```sh
./build-miniaudio-x11/src/xcthugha \
  --audio-output-driver=miniaudio \
  --miniaudio-playback-device "Built-in Output"
```

Live capture can similarly use `--miniaudio-capture-device NAME`.

The help path should print usage before any X11 initialization. Normal runs now
defer display startup until the display initialization section, after option
parsing and core runtime setup.

## Debugging Hooks

The graphical main loop exposes timing through:

- `CTH_TRACE` logging in `src/Application.cc`, `AudioRuntime`, display code,
  and related modules.

Runtime verbosity is controlled with `--verbose` / `-v`; current code treats the
value as a log level:

```text
0 error, 1 warn, 2 info, 5 debug, 10 trace
```

Existing code uses `CTH_TRACE`, `CTH_DEBUG`, `CTH_INFO`, `CTH_WARN`,
`CTH_ERROR`, `CTH_ERRNO`, and older `printfv(...)` style helpers. Follow the
local logging path rather than adding a new one.

## Build-System Gotchas To Preserve

- Startup configuration is built by `src/Configuration.cc`; do not reintroduce
  per-frontend option parser wrappers.
- `xwin_keys.cc` is still an X11 wrapper compile unit for key handling.
- CMake generates `default.keymap.str` under `build/src/`; in-tree builds may
  generate `src/default.keymap.str`.
- Local object files in `src/` are not authoritative. Check source lists in
  `CMakeLists.txt`.

## Porting Strategy

### Phase 1: Keep The Verified X11 Build Green

The current stable slice is CMake plus `xcthugha`.

Recommended practice:

- Use CMake for active development and CI.
- Treat `xcthugha` as the behavioral reference when modernizing.
- Keep generated build output under `build/` or another out-of-tree CMake build
  directory.

### Phase 2: Extend The Modern Audio Runtime

The audio seams are now the only playback/input model:

- `RuntimeFactory`;
- `PcmSourceFactory`;
- `PcmSource`;
- `AudioInput`;
- `AudioOutput`;
- `AudioBuffer`;
- `AudioFrameBuilder`;
- `AudioFrame`.

Good next steps:

- Keep characterizing miniaudio against PulseAudio-compatible output on Linux
  before changing Linux automatic output priority.
- Add native sinks only if miniaudio fails a documented platform requirement.
- Add more targeted tests for raw PCM, EOF/drain handling, and latency-driven
  visual sample selection.
- Keep visual code reading through `audioFrameRawData()` and
  `audioFrameProcessedWaveData()`.

### Phase 3: Continue The Video Filterchain Migration

`VideoFilterchain` has explicit image, border, flame, translate, wave, text,
frame-commit, palette, flashlight, and indexed-frame export filters.
`VideoDirector` updates the current filter objects before each filterchain run.

The video filterchain path passes one `VideoFrame` through the filterchain. The
frame contains the current `CthughaBuffer`, frame context, display palette, and
a final `IndexedFrame` publication slot. `CthughaDisplay` receives that
`IndexedFrame` for source pixels, geometry, pitch, and palette synchronization.

Good next steps:

- Allocate/reallocate display scratch buffers from incoming frame geometry.
- Add tests around deterministic visual stages before changing artistic
  behavior.

### Phase 4: Add A Modern Display Target

The cleanest future frontend is likely SDL2/SDL3 or a similarly thin texture
presenter:

- preserve the indexed 8-bit engine buffer plus palette first;
- render the palette-expanded texture through the new frontend;
- keep presentation size separate from `BUFF_WIDTH`/`BUFF_HEIGHT`;
- preserve X11 behavior as a reference until a modern frontend is usable.

### Phase 5: Add Focused Tests

Highest-value tests:

- `AudioProcessor` modes over fixed 1024-sample frames;
- `AudioAnalyzer` and `AcousticContext` fire/intensity behavior;
- palette loading, metadata, and set filtering;
