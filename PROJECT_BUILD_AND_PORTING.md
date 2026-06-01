# Build and Porting Notes

## Current Build Systems

The project currently has two build paths:

- modern CMake, which is the verified reference path in this workspace;
- old autoconf/automake files, still maintained enough to describe/build the
  X11 frontend.

The CMake files are:

```text
CMakeLists.txt
cmake/config.h.in
cmake/generate_keymap.cmake
src/CMakeLists.txt
```

The autotools files are:

```text
configure.in
configure
Makefile.am
Makefile
src/Makefile.am
src/Makefile
doc/Makefile.am
doc/Makefile
config.h
config.h.in
```

## Modern CMake Build

The normal build command is:

```sh
cmake -S . -B build
cmake --build build
```

The populated `build/` directory currently contains:

- `build/src/xcthugha`

Major CMake options:

- `CTH_BUILD_X11`: build the X11 frontend. Default: `ON`.
- `CTH_ENABLE_PULSE`: enable PulseAudio/PipeWire-Pulse output when
  `libpulse-simple` is available. Default: `ON`.
- `CTH_ENABLE_DSP`: enable OSS `/dev/dsp` support when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_CDROM`: enable CD-ROM controls when headers are available.
  Default: `OFF`.
- `CTH_ENABLE_MIXER`: enable OSS mixer controls when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_MINIMP3`: enable embedded minimp3 decoding. Default: `ON`.
- `CTH_ENABLE_XPM`: enable XPM screenshots when Xpm is available. Default:
  `ON`.
- `CTH_DATA_DIR`: installed runtime data directory. Current cache value:
  `/usr/local/share/cthughanix`.

Current CMake cache/config state in `build/`:

- X11 frontend: enabled.
- Pulse output: enabled and found.
- OSS DSP input/output: enabled.
- OSS mixer: enabled.
- CD-ROM controls: disabled by CMake option.
- minimp3 decoder: enabled.
- XPM screenshots: enabled.
- C compiler: `/usr/bin/cc`.
- C++ compiler: `/usr/bin/c++`.
- CMake version recorded in the build directory: 3.31.6.

## Autotools Build

The old build still uses:

- `configure.in`
- generated `configure`
- generated `Makefile`, `src/Makefile`, and `doc/Makefile`
- `Makefile.am` files in root, `src/`, and `doc/`

Root `Makefile.am` recurses into:

```text
src
```

and adds `doc` only when the generated `BUILD_DOCS` conditional is true.

Current generated autotools target state:

- selected programs: `xcthugha`;
- selected setuid programs: none;
- the old server-mode program is not a current target.

Major `configure.in` options:

- `--disable-xwin`: skip the X11 target.
- `--with-dsp=DEV` / `--without-dsp`: OSS DSP.
- `--without-pulse`: disable PulseAudio-compatible output.
- `--with-cdrom=DEV` / `--without-cdrom`: CD-ROM ioctl support.
- `--with-mixer=DEV` / `--without-mixer`: OSS mixer.
- `--with-arch=ARCH`: old CPU optimization selection.

Removed/stale options from earlier project notes:

- `--disable-svga` is not present.
- `--disable-gl` is not present.
- `--disable-serv` is not present.
- `--without-network` is not present.
- the old server-mode program is not built from current source files.

Current generated autotools config state from `config.h` and generated
Makefiles:

- `WITH_DSP`, `WITH_MIXER`, `WITH_PULSE`, and `WITH_MINIMP3` are enabled.
- `WITH_CDROM` is enabled in the autotools `config.h`.
- `USE_XPM` is enabled.
- `PACKAGE` is `cthughanix`, and `pkglibdir` is `$(libdir)/cthughanix`.
- `TARGETS` is `xcthugha`.
- `TARGETS_SUID` is empty.

`config.log` is from this workspace on Linux/x86_64 with GCC 14.2.0 and was
created by `./configure --no-create --no-recursion`.

## Native Dependencies

Core/common:

- C and C++ compiler;
- CMake for the modern path;
- make/autoconf/automake if regenerating autotools files;
- POSIX process/file APIs;
- curses or ncurses for text UI support;
- gzip at runtime for `.gz` assets.

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
- optional PulseAudio-compatible output via `libpulse-simple`;
- optional OSS `/dev/dsp`;
- optional OSS mixer ioctls;
- optional Linux/Unix CD-ROM ioctls;

## Verified Commands

These commands were run successfully while updating the docs:

```sh
cmake --build build
tests/headers/check-headers.sh
```

`cmake --build build` reconfigured and built all current CMake targets.

`tests/headers/check-headers.sh` reported:

```text
Checked 41 headers.
Skipped: 1.
Failures: 0.
```

In the current headless shell, this command did not reach usage output:

```sh
build/src/xcthugha --help
```

It exited with:

```text
Error: Can't open display:
```

That is expected in a no-`DISPLAY` environment because the X11 frontend
initializes X before normal option help can be shown.

## Debugging Hooks

The graphical main loop has two timing paths:

- `CTH_TRACE` logging in `src/initExitDisp.cc`, `AudioRuntime`, display code,
  and related modules;
- an older local `PROF` block in `src/initExitDisp.cc`, currently behind
  `#undef PROF`.

Runtime verbosity is controlled with `--verbose` / `-v`; current code treats the
value as a log level:

```text
0 error, 1 warn, 2 info, 5 debug, 10 trace
```

Existing code uses `CTH_TRACE`, `CTH_DEBUG`, `CTH_INFO`, `CTH_WARN`,
`CTH_ERROR`, `CTH_ERRNO`, and older `printfv(...)` style helpers. Follow the
local logging path rather than adding a new one.

## Build-System Gotchas To Preserve

- Do not compile `options.cc` directly for every target unless replacing the
  wrapper scheme deliberately.
- `xwin_options.cc` and `xwin_keys.cc` are the remaining wrapper compile units
  for X11-specific options and key handling.
- CMake generates `default.keymap.str` under `build/src/`; in-tree builds
  may generate `src/default.keymap.str`.
- Local object files in `src/` are not authoritative.
  Check source lists in `CMakeLists.txt` and `Makefile.am`.

## Porting Strategy

### Phase 1: Keep The Verified X11 Build Green

The current stable slice is CMake plus `xcthugha`.

Recommended practice:

- Use CMake for active development.
- Keep the autotools files coherent while they remain in the tree, but avoid
  relying on stale build artifacts.
- Treat `xcthugha` as the behavioral reference when modernizing.

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

- Add an ALSA/PipeWire-native input/output path through these interfaces.
- Add targeted tests for raw PCM, EOF/drain handling, and latency-driven visual
  sample selection.
- Keep visual code reading through `audioFrameRawData()` and
  `audioFrameProcessedWaveData()`.

### Phase 3: Continue The Visual Pipeline Migration

`VisualPipeline` has explicit image, border, flame, translate, wave,
frame-commit, palette, and flashlight modules. `VisualDirector` updates the
current stage objects before each pipeline run.

The visual stage path passes one `VisualFrame` through the pipeline. The frame
contains the current `CthughaBuffer`, frame context, and display palette. The
remaining compatibility coupling is in the display path:
`CthughaBuffer::current` still supplies buffer geometry and passive-pixel reads.

Good next steps:

- Move the remaining `CthughaBuffer::current` lookups behind explicit
  display/provider objects.
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
- indexed image metadata/loading/clipping for PCX and PNG;
- built-in translation generator catalog entries;
- keymap parsing with examples from `src/default.keymap`;
- deterministic flame/wave transforms after they are exposed as testable stages.
