# CthughaNix Project Summary

This repository is a modernization/refactoring snapshot of CthughaNix 1.5, the
Linux/Unix continuation of Cthugha-L and descendant of the DOS Cthugha 5.x
visualizer. The code still has its classic indexed-color visual engine, but the
current tree has been substantially reworked since the first project notes were
written.

The current reference application target is `xcthugha`, the X11 frontend. The
modern CMake build produces `xcthugha`, `tabheader`, `tabinfo`, and the
translation-table generator programs under `tab/`. The old autotools build files
are still present and currently select `xcthugha`, `tabheader`, and `tabinfo`.
The old SVGAlib and OpenGL frontend paths have been removed. The old sound
server/network sources are no longer present; only stale object files from
previous builds remain in `src/`.

The best mental model for one frame is now:

```text
audio source -> AudioRuntime / AudioFrame
             -> AudioProcessor
             -> AudioAnalyzer / AcousticContext
             -> AutoChanger
             -> VisualPipeline
             -> CthughaDisplay
             -> DisplayDevice frontend
```

The shared graphical loop is in `src/initExitDisp.cc`. Each frame currently
runs, in order:

```text
CthughaDisplay::nextFrame()
audioFrameTick()
AudioVisualBridge::runFrame()
VisualPipeline::run()
CthughaDisplay::operator()()  # when the frontend asks run() to draw
CDPlayer::operator()()
deferred suspend handling
```

## Documentation Set

- `PROJECT_STRUCTURE.md` maps the current directories, active source groups,
  generated artifacts, build targets, and runtime assets.
- `PROJECT_RUNTIME_MAP.md` traces startup, frame flow, audio flow, visual buffer
  flow, display flow, configuration, and input.
- `PROJECT_SEAMS_AND_RISKS.md` lists useful extension seams and the risky edges
  that remain after the recent refactor.
- `PROJECT_BUILD_AND_PORTING.md` records the current CMake and autotools build
  state, dependencies, verification commands, and porting strategy.
- `PROJECT_MAIN_LOOP_EXPLAINED.md` is a guided source walk through one frame.
- `PROJECT_VERIFICATION.md` records the checks used to update these files.

## Current Project Shape

- `src/` contains the application source: 51 top-level `.cc` files and 41
  top-level headers.
- `tab/` contains 8 table generator sources, 11 `.cmd` descriptors, and local
  build outputs.
- `map/` contains 100 `.map` palettes and `map/png/` contains 23 palette preview
  PNGs.
- `pcx/` contains 12 PCX assets: 6 plain `.pcx` files and 6 gzip-compressed
  copies.
- `external/minimp3/` is the embedded MP3 decoder used by the modern audio path.
- `external/cthugha-js/` is a separate JavaScript port/reference tree.
- `tests/headers/` contains the current local verification script for checking
  source/header self-containment.
- `build/` is a populated CMake build directory and currently contains built
  `xcthugha`, `tabheader`, `tabinfo`, and `tab/cmd_*` binaries.

## Architectural Center

The classic visual domain still revolves around `CoreOption`, the runtime
registry for selectable visual entries. The current active categories include:

- global/display-ish options: `display`, `border`, `flashlight`,
  `sound-processing`;
- per-buffer options: `flame`, `palette`, `pcx`, `translate`, `wave`, `object`,
  `flame-general`, `wave-scale`, and `table`.

Audio and visual control are now separated:

- `AudioRuntime`, `RuntimeFactory`, `PcmSourceFactory`, and `Audio` classes own
  modern source/output composition.
- `AudioFrame` is the facade used by visual code to get the current raw and
  processed 1024-sample window.
- `AudioProcessor` implements `none`, `Filter1`, `Filter2`, and `FFT` for the
  `sound-processing` option.
- `AudioAnalyzer` produces frame-local `AudioAnalysis`; `AcousticContext` keeps
  rolling intensity/fire state for effects and automatic changes.
- `AudioVisualBridge` runs processing, analysis, and `AutoChanger` policy before
  visual mutation.
- `VisualPipeline` is the visual-stage executor. One-shot PCX image overlay,
  flashlight, border, flame, translate, wave, frame commit, and palette
  smoothing now run as explicit modules. `VisualDirector` synchronizes the
  selected buffer and updates stage bindings for selected images, per-buffer
  effects, border mode, and palette state before each run.

## Highest-Value Seams

- Audio composition seam: `RuntimeFactory`, `PcmSourceFactory`, `PcmSource`,
  `AudioInput`, `AudioOutput`, and `AudioInputProcessor`.
- Audio-to-visual seam: `AudioFrame`, `AudioProcessor`, `AudioAnalyzer`, and
  `AudioVisualBridge`.
- Visual pipeline seam: `VisualDirector`, `VisualPipelineFactory`,
  `VisualPipeline`, `VisualModule`, `VisualFrameContext`, and `CthughaBuffer`.
- Classic visual effect seam: `CoreOptionEntry` / `CoreOptionEntryList`, with
  flame, translate, and wave entries now receiving explicit `CthughaBuffer&`
  objects during pipeline execution.
- Display frontend seam: `DisplayDevice` plus the X11 `CthughaDisplay`
  subclass.
- Asset seams: `.map` palettes, `.pcx`/`.pcx.gz` images, `.cmd` table
  descriptors, `.tab` binary translation tables, and optional `.obj` line
  objects.
- Control seam: `Keymap` action registry and `Interface` screens.
- Build seam: wrapper source files such as `xwin_options.cc` compile shared
  implementations with target-specific macros.

## Current State Notes

The verified build path is CMake:

```sh
cmake --build build
tests/headers/check-headers.sh
```

Both completed successfully in this workspace. A direct attempt to run
`build/src/xcthugha --help` in the current headless shell failed with
`Error: Can't open display:`, which confirms the binary starts through X11
initialization before it can print help.

The project is portable in a transitional sense, not yet a modern clean-room
port. It still carries X11/Xt/Xaw, MIT-SHM, OSS `/dev/dsp`, OSS mixer, CD-ROM
ioctl, shell-based asset helpers, and many global singletons. The refactor has
created better audio and visual seams, and the old monolithic
flame/translate/wave loop has been decomposed into pipeline stages, but the
classic engine is still stateful and global.
