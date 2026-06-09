# Cthugha-X SDL3/Miniaudio Development Release Notes

## Overview

This release moves Cthugha-X from an X11-centered porting effort to a portable
application runtime with an SDL3 development frontend and miniaudio native audio
support.

The old X11 path is still available and important. The new default development
shape is now:

```text
SDL3 frontend + miniaudio audio + application-owned runtime modules
```

## Platform Status

- Linux/GNOME: SDL3 frontend compiles and runs.
- macOS: SDL3 frontend compiles and runs.
- Windows: not yet tested.
- X11: retained as a compatibility frontend through `xcthugha`.

The SDL3 executable target is `cthugha`. The X11 compatibility executable
target is `xcthugha`.

## Highlights

### SDL3 Frontend

The project now has a real SDL3 display frontend, wired through the same
`DisplaySystem` and `DisplayDriverFactory` path as X11. SDL3 handles window
creation, event processing, renderer texture presentation, resize handling,
keyboard input, and optional frame dumping.

SDL3 is now the preferred frontend for current development, performance work,
and portable display work. X11 remains available for compatibility, the X11
panel, MIT-SHM/pixmap behavior, and old Unix display environments.

### Native Audio Through Miniaudio

Miniaudio is now the preferred portable native audio path. It provides native
playback and capture without requiring PulseAudio or OSS device paths.

The runtime can still use PulseAudio-compatible output, OSS DSP output, or
`AudioNullOutput`, but miniaudio is the path to use for modern Linux and macOS
testing.

Useful runtime switches:

```sh
--audio-output-driver=miniaudio
--miniaudio-target-latency-ms N
--miniaudio-playback-device NAME
--miniaudio-capture-device NAME
```

### Application-Owned Runtime Modules

The codebase has been heavily restructured around explicit ownership instead of
ambient globals and frontend-side control.

`Application` now owns the major runtime roots:

- `AudioIngest`
- `SceneRuntime`
- `FrameGeneratorRuntime`
- `DisplaySystem`
- `RuntimeChangeMediator`
- `RuntimePersistence`
- `RuntimeShutdown`
- `PlatformLifecycle`

The frame path is now much clearer:

```text
AudioIngest
DefaultAudioFramePipeline
SceneChangeScheduler
FrameGeneratorRuntime
FrameFilterchain
CthughaDisplay
DisplayRuntime
```

This makes the SDL3 frontend possible without dragging the visual engine,
scene state, audio runtime, and display backend through the old X11-era global
shape.

## SDL3 Development Build

Use this build for current development on Linux/GNOME and macOS:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF

cmake --build build --target cthugha
```

Run:

```sh
./build/src/cthugha --display-driver=sdl3 --audio-output-driver=miniaudio
```

For file playback:

```sh
./build/src/cthugha \
  --display-driver=sdl3 \
  --audio-output-driver=miniaudio \
  --play /path/to/audio.wav
```

## X11 Compatibility Build

Use this build to verify the retained X11 frontend:

```sh
cmake -S . -B build-x11 -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCTH_BUILD_X11=ON \
  -DCTH_BUILD_SDL3=OFF \
  -DCTH_ENABLE_MINIAUDIO=ON \
  -DCTH_ENABLE_PULSE=OFF \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF

cmake --build build-x11 --target xcthugha
```

Run:

```sh
./build-x11/src/xcthugha --display-driver=x11
```

In virtualized, forwarded, or nested X11 environments, MIT-SHM may need to be
disabled:

```sh
./build-x11/src/xcthugha --display-driver=x11 --no-mit-shm
```

## Legacy Unix Compatibility Build

Use this when checking the old Unix/X11 shape with traditional audio surfaces:

```sh
cmake -S . -B build-legacy-x11 -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCTH_BUILD_X11=ON \
  -DCTH_BUILD_SDL3=OFF \
  -DCTH_ENABLE_MINIAUDIO=OFF \
  -DCTH_ENABLE_PULSE=ON \
  -DCTH_ENABLE_DSP=ON \
  -DCTH_ENABLE_MIXER=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF

cmake --build build-legacy-x11 --target xcthugha
```

## Verification

Recommended focused checks:

```sh
ctest --test-dir build --output-on-failure
```

For SDL3 presentation and scene-script coverage:

```sh
cmake --build build \
  --target cthugha scene_script_test sdl3_presentation_test \
  --parallel

ctest --test-dir build \
  -R '^(scene_script_test|sdl3_presentation_test)$' \
  --output-on-failure
```

## Notes And Caveats

- SDL3 is the current development frontend, but the bare CMake defaults remain
  compatibility-biased toward X11. Use the explicit SDL3 build flags above.
- Windows has not been tested yet.
- Some X11-era display compatibility globals remain, especially around mapped
  image memory, text layout, and the X11 panel. They are no longer the frontend
  ownership boundary, but they are still cleanup targets.
- PulseAudio and OSS support remain useful compatibility paths, but miniaudio
  is the native audio path for modern portable testing.
