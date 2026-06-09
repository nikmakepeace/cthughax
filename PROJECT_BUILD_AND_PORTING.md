# Build And Porting Notes

## Build System

CMake is the active build system.

Primary build files:

```text
CMakeLists.txt
cmake/config.h.in
cmake/generate_keymap.cmake
src/CMakeLists.txt
tests/CMakeLists.txt
tests/benchmarks/CMakeLists.txt
```

The normal local build is:

```sh
cmake -S . -B build
cmake --build build
```

The main executable target is `xcthugha` when `CTH_BUILD_X11=ON`.

## CMake Options

Current root options:

- `CTH_BUILD_X11`: build the X11 frontend. Default `ON`.
- `CTH_BUILD_SDL3`: display option placeholder. Default `OFF`; not wired to a
  target today.
- `CTH_BUILD_TESTS`: build and register focused tests. Default `ON`.
- `CTH_BUILD_BENCHMARKS`: build Google Benchmark suites. Default `OFF`.
- `CTH_RUN_AUDIO_DEVICE_TESTS`: register smoke tests that open real audio
  devices. Default `OFF`.
- `CTH_ENABLE_PULSE`: enable PulseAudio-compatible output when
  `libpulse-simple` is found. Default `ON`.
- `CTH_ENABLE_DSP`: enable OSS `/dev/dsp` support when soundcard headers are
  present. Default `ON`.
- `CTH_ENABLE_MIXER`: enable OSS mixer controls when soundcard headers are
  present. Default `ON`.
- `CTH_ENABLE_MINIMP3`: enable vendored minimp3 decoding. Default `ON`.
- `CTH_ENABLE_MINIAUDIO`: enable vendored miniaudio playback/capture. Default
  `ON`.
- `CTH_MINIAUDIO_NO_RUNTIME_LINKING`: Apple-only miniaudio build mode that
  defines `MA_NO_RUNTIME_LINKING` and links CoreFoundation, CoreAudio, and
  AudioToolbox explicitly. Default `OFF`.
- `CTH_DEV_DSP`: default OSS DSP path. Default `/dev/dsp`.
- `CTH_DEV_MIXER`: default OSS mixer path. Default `/dev/mixer`.
- `CTH_DATA_DIR`: installed runtime data directory. Default
  `${CMAKE_INSTALL_FULL_DATADIR}/cthughanix`.

CMake writes generated `config.h` to the build directory and generates
`default.keymap.str` from `src/default.keymap`.

## Dependencies

Core build:

- C and C++ compiler;
- CMake 3.16 or newer;
- POSIX headers/functions where available;
- threads;
- math library.

X11 target:

- X11;
- Xext;
- Xt;
- Xaw;
- Xmu;
- Zlib.

Audio/media:

- `external/miniaudio` for optional playback/capture;
- `external/minimp3` for optional MP3 decode;
- optional `libpulse-simple`;
- optional OSS soundcard headers;
- optional OSS mixer ioctl support.

Miniaudio is vendored and does not require a system miniaudio package. On Apple,
the default build lets miniaudio perform runtime framework loading; use
`CTH_MINIAUDIO_NO_RUNTIME_LINKING=ON` when an app-bundle/signing path requires
explicit framework linkage.

## Useful Build Commands

X11 with tests:

```sh
cmake -S . -B build -G Ninja \
  -DCTH_BUILD_X11=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build --target xcthugha
ctest --test-dir build --output-on-failure
```

Miniaudio path without Pulse:

```sh
cmake -S . -B build-miniaudio-x11-no-pulse -G Ninja \
  -DCTH_BUILD_X11=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-miniaudio-x11-no-pulse --target xcthugha
```

Headless/unit-focused miniaudio build:

```sh
cmake -S . -B build-miniaudio-headless -G Ninja \
  -DCTH_BUILD_X11=OFF \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_ENABLE_DSP=OFF \
  -DCTH_ENABLE_MIXER=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-miniaudio-headless
ctest --test-dir build-miniaudio-headless --output-on-failure
```

Real-device miniaudio smoke tests:

```sh
cmake -S . -B build-miniaudio-device-tests -G Ninja \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_RUN_AUDIO_DEVICE_TESTS=ON \
  -DCTH_BUILD_TESTS=ON
cmake --build build-miniaudio-device-tests
ctest --test-dir build-miniaudio-device-tests \
  -R 'miniaudio_.*_smoke' --output-on-failure
```

Those smoke tests are intentionally opt-in and should fail if miniaudio opens
its Null backend instead of a real device.

## Runtime Audio Selection

Useful runtime switches:

```sh
--audio-output-driver auto|null|pulse|oss|miniaudio
--pulse-server SERVER
--pulse-latency-ms N
--miniaudio-target-latency-ms N
--miniaudio-playback-device NAME
--miniaudio-capture-device NAME
--audio-output-dump FILE
```

On platforms where automatic selection prefers miniaudio, miniaudio playback is
tried before Pulse. On other platforms, Pulse is tried before miniaudio when
both are available. OSS playback is only an automatic fallback when miniaudio is
not compiled in.

`AudioNullOutput` is non-realtime. It is useful for silent/null runs and dump
paths, but it does not prove an audible backend opened.

## Diagnostics

Runtime verbosity is controlled by `--verbose` / `-v`. The common levels are:

```text
0 error
1 warn
2 info
5 debug
10 trace
```

Trace/debug logs are emitted through `LogSink` and legacy `CTH_*` macros. Useful
trace contexts include `audio timing`, `audio runtime`, `frame timing`,
`frame pacing`, `frame generator`, `frame filterchain`, and `display timing`.

The frame-filterchain add-stage diagnostic now includes a human-readable filter
name and pointer, for example:

```text
frame filterchain: added stage=3 filter=flame ptr=0x... owned=1 mode=0 size=3
```

## Porting Strategy

### Keep X11 As The Reference

`xcthugha` is the only wired graphical executable. Preserve its behavior while
lifting dependencies behind `DisplaySystem`, `DisplayRuntime`, and
`CthughaDisplay`.

### Prefer The Current Audio Seams

New audio inputs should be `PcmSource` implementations. New outputs should be
`AudioOutput` implementations selected by `RuntimeFactory`. Keep decoded
history and visual frame construction in `AudioIngest`.

### Preserve Indexed-Frame Presentation

The portable display seam is:

```text
IndexedFrame + FramePalette -> CthughaDisplay/PresentationComposer -> DisplayRuntime
```

A new frontend should present indexed pixels and palette-expanded output
without pulling scene or audio ownership into the display backend.

### Isolate Remaining Display Globals

Before or during a new frontend, reduce direct dependence on X11-era display
globals such as `disp_size`, `bypp`, `bytes_per_line`, `draw_mode`,
`text_size`, and `fontSize`.

### Grow Tests Around Behavior

The current test tree already covers many small seams. Add focused tests for
new runtime command behavior, audio timing, frame-generator filters, display
presentation, or asset parsing before changing behavior that users can see or
hear.
