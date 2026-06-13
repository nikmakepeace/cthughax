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

The current development target is the SDL3 frontend with miniaudio. Build it
into the normal `./build` directory with:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build --target cthugha
```

The SDL3 executable target is `cthugha`. The X11 executable target is
`xcthugha` when `CTH_BUILD_X11=ON`.

The bare CMake option defaults remain compatibility-biased toward X11. Use the
explicit SDL3 command above for current development work.

## CMake Options

Current root options:

- `CTH_BUILD_X11`: build the X11 frontend and `xcthugha` compatibility target.
  Default `ON`.
- `CTH_BUILD_SDL3`: build the SDL3 frontend and `cthugha` development target.
  Default `OFF`.
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

SDL3 target:

- SDL3;
- Zlib.

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

SDL3 development build:

```sh
cmake -S . -B build -G Ninja \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build --target cthugha
ctest --test-dir build --output-on-failure
```

macOS app-bundle package:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_MINIAUDIO_NO_RUNTIME_LINKING=ON \
  -DCTH_BUILD_WX_PANEL=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build --target cthugha_macos_app
```

The package target writes `build/dist/CthughaX.app` and
`build/dist/CthughaX Panel.app`. The main bundle uses
`resources/macos/cthughax.icns`; the panel bundle uses
`resources/macos/cthughax-panel.icns`. The bundles include their non-system
shared libraries under `Contents/Frameworks` with install names rewritten to
bundle-relative paths. `CthughaX.app` also includes the source `resources`
directory and a nested `CthughaX Panel.app` so the in-app control-panel shortcut
works even when only the main app bundle is copied. The separate
`CthughaX Panel.app` is also provided beside the main app for direct launch with
its own Finder/Dock identity.
On Apple builds, the bundles are ad-hoc signed by default through
`CTH_MACOS_CODESIGN_IDENTITY=-`; pass `-DCTH_MACOS_CODESIGN_IDENTITY=` to skip
signing or set a Developer ID identity for a distributable signed build.

X11 compatibility build:

```sh
cmake -S . -B build-x11 -G Ninja \
  -DCTH_BUILD_X11=ON \
  -DCTH_BUILD_SDL3=OFF \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-x11 --target xcthugha
ctest --test-dir build-x11 --output-on-failure
```

Legacy Unix compatibility build:

```sh
cmake -S . -B build-legacy-x11 -G Ninja \
  -DCTH_BUILD_X11=ON \
  -DCTH_BUILD_SDL3=OFF \
  -DCTH_ENABLE_MINIAUDIO=OFF \
  -DCTH_ENABLE_PULSE=ON \
  -DCTH_ENABLE_DSP=ON \
  -DCTH_ENABLE_MIXER=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF
cmake --build build-legacy-x11 --target xcthugha
ctest --test-dir build-legacy-x11 --output-on-failure
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
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=OFF \
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

### Keep SDL3 As The Development Target

`cthugha` is the active graphical executable for new frontend and performance
work. Prefer SDL3/miniaudio builds for day-to-day development, timing traces,
scene-script benchmarking, and macOS/Linux portability work.

### Preserve X11 Compatibility

`xcthugha` remains the X11 compatibility frontend. Keep it buildable and use it
to guard behavior specific to the X11 path, especially the X11 panel,
MIT-SHM/pixmap presentation details, and old Unix-window-manager behavior.

### Preserve Legacy Unix Compatibility

The legacy compatibility shape is X11 plus the traditional Unix audio surfaces:
Pulse when available, OSS `/dev/dsp` and `/dev/mixer` when headers are present,
and no SDL3 dependency. Use this build to check that portability work has not
silently made the old deployment path depend on SDL3, miniaudio-only behavior,
or platform APIs outside the historical Unix/X11 stack.

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

The new SDL3 frontend no longer depends on the old ambient display ownership
globals such as a process-wide `DisplayDevice*`, `CthughaDisplay*`, or global
framebuffer alias. `Application` opens a selected driver through
`DisplaySystem`, and both frontends use `DisplayRuntime`/`CthughaDisplay` for
presentation.

There are still transitional compatibility globals in `DisplayDevice.h`:
`disp_size`, `bypp`, `bytes_per_line`, `draw_mode`, `text_size`, and
`fontSize`. X11 still uses them heavily for mapped image memory, font layout,
and the Xaw panel. SDL3 initializes a subset of them so shared text rendering
and legacy `DisplayDevice` helpers continue to work, but they should no longer
be treated as the frontend boundary.

Next display cleanup should move those values behind backend-owned geometry,
pixel-format, and overlay-layout objects, leaving narrow X11-specific
compatibility state in the X11 implementation. Palette globals in `display.h`
(`bitmap_colors*` and `rev_byte_order`) are a separate remaining legacy seam.

### Grow Tests Around Behavior

The current test tree already covers many small seams. Add focused tests for
new runtime command behavior, audio timing, frame-generator filters, display
presentation, or asset parsing before changing behavior that users can see or
hear.
