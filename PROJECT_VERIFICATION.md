# Verification Log

This file records what was checked while updating the root `PROJECT_*.md` files.

## Tooling Notes

`rg` and `git` are not installed in this execution environment. File inventory
and source searches used `find`, `grep`, `sed`, `wc`, CMake, and local scripts
instead.

Because `git` is unavailable, repository history and dirty-state inspection were
not verified here. A `.git/` directory exists in the workspace, but no Git
commands were available.

## Root Project Docs Read

Files read before editing:

- `PROJECT_SUMMARY.md`
- `PROJECT_STRUCTURE.md`
- `PROJECT_BUILD_AND_PORTING.md`
- `PROJECT_RUNTIME_MAP.md`
- `PROJECT_SEAMS_AND_RISKS.md`
- `PROJECT_MAIN_LOOP_EXPLAINED.md`
- `PROJECT_VERIFICATION.md`

Major stale themes found:

- old `SoundAnalyze` / `SoundProcess` names;
- old `SoundServer`, `SoundDeviceNet`, `network`, and `cthugha-server` paths;
- obsolete generated target list that still included the server;
- old asset counts;
- old build notes that predated the working CMake build;
- partial updates in `PROJECT_MAIN_LOOP_EXPLAINED.md` that mixed old and new
  architecture.

## Repository and Inventory

Commands used:

```sh
ls -la
find . -maxdepth 1 -name 'PROJECT_*.md' -print
wc -l PROJECT_*.md
find src -maxdepth 1 -type f -name '*.cc' -print | wc -l
find src -maxdepth 1 -type f -name '*.h' -print | wc -l
find map -maxdepth 1 -type f -name '*.map' -print | wc -l
find map/png -maxdepth 1 -type f -name '*.png' -print | wc -l
find pcx -maxdepth 1 -type f -print | wc -l
find tab -maxdepth 1 -type f -name '*.cmd' -print | wc -l
```

Current findings:

- Root `PROJECT_*.md` files: 7.
- `src/` top-level `.cc` files: 71.
- `src/` top-level headers: 41.
- `map/` `.map` palettes: 100.
- `map/png/` palette preview PNGs: 23.
- `pcx/` files: 12.
- `tab/` `.cmd` descriptors: 11.
- `external/minimp3/` contains the embedded MP3 decoder headers/license.
- `external/cthugha-js/` is present as a separate JavaScript port/reference.
- `build/` is a populated CMake build directory.
- Local `.o` files exist in source directories; some are stale relative to
  current source.

## Build Files Checked

Files read:

- `CMakeLists.txt`
- `src/CMakeLists.txt`
- `tab/CMakeLists.txt`
- `cmake/config.h.in`
- `Makefile.am`
- `src/Makefile.am`
- `tab/Makefile.am`
- `configure.in`
- generated `Makefile`
- generated `src/Makefile`
- generated `tab/Makefile`
- generated `doc/Makefile`
- `config.h`
- `config.log`
- `build/CMakeCache.txt`
- `build/config.h`

Commands used:

```sh
grep -n "AC_ARG_ENABLE\|AC_ARG_WITH\|TARGETS\|PULSE\|MINIMP3" configure.in
grep -n "#define \(WITH_\|HAVE_\|USE_\|CTH_\|DEV_\|PACKAGE\|VERSION\|WORDS\)" config.h
grep -n "^TARGETS\|^TARGETS_SUID\|^bin_PROGRAMS\|^SUBDIRS\|^pkglibdir\|^CXXFLAGS\|^PULSE\|^X_LIBS\|^GL_LIBS\|^S_LIBS\|^N_LIBS" Makefile src/Makefile tab/Makefile doc/Makefile
grep -n "^CTH_\|^WITH_\|^PULSE\|^X11\|^ZLIB\|^CMAKE_C\|^CURSES" build/CMakeCache.txt
```

Findings:

- CMake builds `xcthugha`, `tabheader`, `tabinfo`, and `tab/cmd_*` tools.
- CMake does not build `cthugha` or `glcthugha`.
- CMake option state in `build/`: X11 ON, tab tools ON, Pulse ON/found, DSP ON,
  mixer ON, minimp3 ON, XPM ON, CD-ROM OFF.
- Current generated autotools `TARGETS` is `xcthugha tabheader tabinfo`.
- Current generated autotools `TARGETS_SUID` is empty.
- `configure.in` has no current server/network option.
- `config.log` is from Linux/x86_64 in this workspace, with GCC 14.2.0, created
  by `./configure --no-create --no-recursion`.

## Build and Check Commands

Commands run:

```sh
cmake --build build
tests/headers/check-headers.sh
make -n all
build/src/xcthugha --help
build/src/tabinfo
build/src/tabheader
build/tab/cmdRead
```

Results:

- `cmake --build build`: succeeded and built all current CMake targets.
- `tests/headers/check-headers.sh`: succeeded.
- Header check output:

  ```text
  Checked 41 headers.
  Skipped: 1.
  Failures: 0.
  ```

- `make -n all`: succeeded as an autotools dry run.
- `build/src/xcthugha --help`: failed in the headless shell with
  `Error: Can't open display:`. This verifies the binary exists and starts X11
  initialization before it can show help.
- `build/src/tabinfo`: printed `Syntax: tabinfo <filename>` and exited nonzero,
  as expected with no argument.
- `build/src/tabheader`: printed `Can't read header.` and exited nonzero, as
  expected with no input.
- `build/tab/cmdRead`: printed usage and exited nonzero, as expected with no
  arguments.

## Entrypoint and Runtime Tracing

Commands used:

```sh
grep -R -n "int main" src tab tests tools
grep -R -n "SoundAnalyze\|AudioAnalyzer\|AudioRuntime\|AudioVisualBridge\|AudioProcessor\|PcmSourceFactory\|RuntimeFactory\|VisualPipeline\|VisualDirector" src CMakeLists.txt cmake tests PROJECT_*.md
grep -R -n "SoundServer\|soundServer\|cthugha-server\|WITH_NETWORK\|network" src/*.cc src/*.h CMakeLists.txt src/CMakeLists.txt configure.in Makefile.am src/Makefile.am PROJECT_*.md
```

Files read:

- `src/initExitDisp.cc`
- `src/AudioRuntime.*`
- `src/RuntimeFactory.*`
- `src/PcmSourceFactory.*`
- `src/Audio.*`
- `src/AudioFrame.*`
- `src/AudioVisualBridge.*`
- `src/AudioProcessor.*`
- `src/AudioAnalyzer.*`
- `src/AutoChanger.*`
- `src/VisualPipeline.*`
- `src/VisualDirector.*`
- `src/CthughaFrameBuffer.*`
- `src/CthughaBuffer.*`

Findings:

- Graphical startup and frame scheduling are in `src/initExitDisp.cc`.
- Current frame order is `nextFrame`, `audioFrameTick`, bridge, visual pipeline,
  optional display, CD update, suspend handling.
- `AudioVisualBridge` constructs/destroys `AutoChanger`.
- `SoundAnalyze.*` and `SoundProcess.cc` are not current source files.
- `SoundServer.*`, `SoundDeviceNet.cc`, `network.*`, and `serv_main.cc` are not
  current source files.

## Audio Pipeline Tracing

Files read:

- `src/Audio.h`
- `src/Audio.cc`
- `src/AudioRuntime.cc`
- `src/RuntimeFactory.cc`
- `src/PcmSourceFactory.cc`
- `src/AudioFrame.cc`
- `src/AudioProcessor.cc`
- `src/AudioAnalyzer.cc`
- `src/SoundDevice.*`
- `src/SoundDeviceDSP.cc`
- `src/SoundDeviceFile.cc`
- `src/SoundDeviceFork.cc`
- `src/SoundDeviceRandom.cc`
- `src/sound.cc`
- `src/Mixer.cc`
- `src/CDPlayer.*`

Cross-checks:

- `PcmSourceFactory` selects line-in, random, WAV, MP3, raw-file, or unknown
  strategies.
- `AudioRuntime` uses a native file pipeline for WAV and MP3 when minimp3 is
  enabled.
- `AudioRuntime` uses `AudioInputProcessor` for native live/random sources when
  available.
- `AudioRuntime` falls back to legacy `SoundDevice` implementations when native
  creation fails or the strategy is unsupported.
- Visual code should read through `audioFrameData()` and
  `audioFrameProcessedData()`.
- `AudioProcessor` owns the former sound-processing modes.
- `AudioAnalyzer` owns frame analysis and `AcousticContext` owns rolling
  intensity/fire state.

## Visual and Display Pipeline Tracing

Files read:

- `src/VisualPipeline.*`
- `src/VisualDirector.*`
- `src/CthughaFrameBuffer.*`
- `src/CthughaBuffer.*`
- `src/Border.*`
- `src/Flashlight.*`
- `src/flames.*`
- `src/waves.*`
- `src/translate.*`
- `src/palettes.cc`
- `src/pcx.*`
- `src/display.cc`
- `src/CthughaDisplay.*`
- `src/DisplayDevice.*`
- `src/DisplayDeviceX11.cc`
- `src/DisplayDeviceX11-Panel.cc`
- retained SVGAlib/OpenGL display source files

Cross-checks:

- `VisualPipelineFactory` currently installs flashlight, border, legacy buffer
  transform, null placeholders, and palette smoothing.
- `LegacyBufferTransformModule` calls `CthughaBuffer::run()`.
- `CthughaBuffer::run()` now does only `flame`, `translate`, `wave`, and swap.
- Flashlight, border, and palette smoothing are outside `CthughaBuffer::run()`.
- X11 display flow still maps `passive_buffer` through `screen()` into
  `CthughaDisplay` buffers and then to `DisplayDeviceX11`.

## Option, Config, UI, and Keymap Tracing

Files read:

- `src/options.cc`
- `src/IniFiles.cc`
- `src/Option.*`
- `src/CoreOption.*`
- `src/Interface.*`
- `src/InterfaceHelp.cc`
- `src/InterfaceCredits.cc`
- `src/InterfaceList.cc`
- `src/keymap.*`
- `src/default.keymap`
- wrapper files `xwin_options.cc`, `svga_options.cc`, `GL_options.cc`,
  `nonx_options.cc`, `xwin_keys.cc`, `nonx_keys.cc`, `GL_keys.cc`

Cross-checks:

- `--ini-file` is an early option and overrides normal ini search.
- Normal ini search includes installed, auto, user, local, extra path, and X11
  resource sources.
- Ini entries are validated and unknown directives are warned.
- `audioProcessing` is written to `.cthugha.auto`.
- Keymap actions exist for `sound-processing`, `border`, and `flashlight`.
- Wrapper files confirm macro-variant compilation by directly including `.cc`
  implementations.

## Asset Format Checks

Commands used:

```sh
find map -maxdepth 1 -type f -name '*.map' -print
find pcx -maxdepth 1 -type f -print
find tab -maxdepth 1 -type f -print
grep -n "CoreOption::load\|CTH_LIBDIR\|extra_lib_path\|map/\|pcx/\|tab/\|obj/\|gzip\|popen\|systemf" src/CoreOption.cc src/palettes.cc src/pcx.cc src/translate.cc src/waves.cc src/misc.cc
```

Findings:

- `.map` palette search path is current directory, `map/`, installed
  `CTH_LIBDIR/map/`, then `--path DIR/map/`.
- Palette metadata supports `name`, `set`, and `energy` before RGB data.
- PCX search path is current directory, `pcx/`, installed `CTH_LIBDIR/pcx/`,
  then `--path DIR/pcx/`.
- `.cmd`/`.tab` search path is current directory, `tab/`, installed
  `CTH_LIBDIR/tab/WIDTHxHEIGHT/`, installed `CTH_LIBDIR/tab/`, then
  `--path DIR/tab/`.
- Object search path is current directory, `obj/`, installed `CTH_LIBDIR/obj/`,
  then `--path DIR/obj/`.
- `.gz` asset loading still shells out to `gzip -cd`.

## Risk Search

Commands used:

```sh
grep -n "mpg123\|l3dec\|xmp\|mkfifo\|popen\|systemf\|fork\|execv\|/bin/sh\|fifo" src/SoundDeviceFile.cc src/SoundDeviceFork.cc src/translate.cc src/CoreOption.cc src/AutoChanger.cc
grep -R -n "HAVE_X11_EXTENSIONS_XSHM_H\|XShm" src/*.cc src/*.h cmake/config.h.in config.h.in
```

Findings:

- Legacy external command paths remain in file playback, translation loading,
  gzip asset loading, and silence messages.
- Legacy file playback now uses `mkdtemp()` plus fifo paths rather than the
  older `tmpnam()` pattern.
- X11 MIT-SHM is included and used directly in the X11 frontend.
- SVGAlib and OpenGL source remain high-risk legacy paths, but CMake does not
  build them.
