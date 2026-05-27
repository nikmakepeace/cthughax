# Build and Porting Notes

## Current Build Systems

The project currently has two build paths:

- modern CMake, which is the verified reference path in this workspace;
- old autoconf/automake files, still maintained enough to describe/build the
  legacy target layout.

The CMake files are:

```text
CMakeLists.txt
cmake/config.h.in
cmake/generate_keymap.cmake
src/CMakeLists.txt
tab/CMakeLists.txt
```

The autotools files are:

```text
configure.in
configure
Makefile.am
Makefile
src/Makefile.am
src/Makefile
tab/Makefile.am
tab/Makefile
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
- `build/src/tabheader`
- `build/src/tabinfo`
- `build/tab/cmd_huricn`
- `build/tab/cmd_smoke`
- `build/tab/cmd_space`
- `build/tab/cmd_gentable`
- `build/tab/cmd_bighalfwheel`
- `build/tab/cmd_downspiral`
- `build/tab/cmd_randswirls`
- `build/tab/cmdRead`

Major CMake options:

- `CTH_BUILD_X11`: build the X11 frontend. Default: `ON`.
- `CTH_BUILD_TAB_TOOLS`: build/install `tab/cmd_*` helper tools. Default: `ON`.
- `CTH_ENABLE_PULSE`: enable PulseAudio/PipeWire-Pulse output when
  `libpulse-simple` is available. Default: `ON`.
- `CTH_ENABLE_DSP`: enable OSS `/dev/dsp` support when soundcard headers are
  available. Default: `ON`.
- `CTH_ENABLE_CDROM`: enable legacy CD-ROM controls when headers are available.
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
- generated `Makefile`, `src/Makefile`, `tab/Makefile`, and `doc/Makefile`
- `Makefile.am` files in root, `src/`, `tab/`, and `doc/`

Root `Makefile.am` recurses into:

```text
src tab
```

and adds `doc` only when the generated `BUILD_DOCS` conditional is true.

Current generated autotools target state:

- selected programs: `xcthugha tabheader tabinfo`;
- selected setuid programs: none;
- `cthugha` and `glcthugha` remain `EXTRA_PROGRAMS`, not selected in the
  current generated Makefiles;
- `cthugha-server` is not a current target.

Major `configure.in` options:

- `--disable-svga`: skip the SVGAlib console target.
- `--disable-xwin`: skip the X11 target.
- `--disable-gl`: skip the OpenGL/GLUT target.
- `--disable-tabtools`: skip `tabheader` and `tabinfo`.
- `--with-dsp=DEV` / `--without-dsp`: OSS DSP.
- `--without-pulse`: disable PulseAudio-compatible output.
- `--with-cdrom=DEV` / `--without-cdrom`: CD-ROM ioctl support.
- `--with-mixer=DEV` / `--without-mixer`: OSS mixer.
- `--without-mpg123`: prefer `l3dec` in the legacy decoder path.
- `--with-arch=ARCH`: old CPU optimization selection.

Removed/stale options from earlier project notes:

- `--disable-serv` is not present.
- `--without-network` is not present.
- `cthugha-server` is not built from current source files.

Current generated autotools config state from `config.h` and generated
Makefiles:

- `WITH_DSP`, `WITH_MIXER`, `WITH_PULSE`, and `WITH_MINIMP3` are enabled.
- `WITH_CDROM` is enabled in the autotools `config.h`.
- `USE_XPM` is enabled.
- `PACKAGE` is `cthughanix`, and `pkglibdir` is `$(libdir)/cthughanix`.
- `TARGETS` is `xcthugha tabheader tabinfo`.
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
- optional legacy external `mpg123`, `l3dec`, and `xmp` paths in
  `SoundDeviceFile`.

Legacy/non-reference frontends:

- SVGAlib requires `libvga`, `libvgagl`, and historically setuid console
  access.
- OpenGL requires OpenGL, GLU, GLUT, and old paletted-texture assumptions.

## Verified Commands

These commands were run successfully while updating the docs:

```sh
cmake --build build
tests/headers/check-headers.sh
make -n all
```

`cmake --build build` reconfigured and built all current CMake targets.

`tests/headers/check-headers.sh` reported:

```text
Checked 41 headers.
Skipped: 1.
Failures: 0.
```

`make -n all` completed its recursive dry run and reported nothing to build in
`src` or `tab`.

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
- `xwin_options.cc`, `svga_options.cc`, `GL_options.cc`, and
  `nonx_options.cc` produce different variants.
- `xwin_keys.cc`, `nonx_keys.cc`, and `GL_keys.cc` produce different key
  handling variants.
- `nonGL_stubs.cc` is needed for non-GL targets because shared code references
  GL option globals.
- CMake generates `default.keymap.str` under `build/src/`; legacy in-tree builds
  may generate `src/default.keymap.str`.
- CMake currently builds only the X11 frontend, even though SVGAlib/OpenGL source
  files still exist.
- Local object files in `src/` and `tab/` are not authoritative. Check source
  lists in `CMakeLists.txt` and `Makefile.am`.

## Porting Strategy

### Phase 1: Keep The Verified X11 Build Green

The current stable slice is CMake plus `xcthugha`.

Recommended practice:

- Use CMake for active development.
- Keep the autotools files coherent while they remain in the tree, but avoid
  relying on stale build artifacts.
- Keep SVGAlib and OpenGL as source-preserved legacy frontends until they are
  intentionally revived or removed.
- Treat `xcthugha` as the behavioral reference when modernizing.

### Phase 2: Continue The Audio Migration

The new audio seams are already in place:

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
- Reduce fallback dependence on `SoundDeviceFile` for raw/MOD/legacy decoder
  cases.
- Keep visual code reading through `audioFrameData()` and
  `audioFrameProcessedData()` rather than directly from `soundDevice`.

### Phase 3: Continue The Visual Pipeline Migration

`VisualPipeline` exists, but the actual flame/translate/wave work still happens
inside `CthughaBuffer::run()` through the `LegacyBufferTransformModule`.

Good next steps:

- Move flame, translate, wave, swap, and palette smoothing into explicit
  `VisualModule` implementations one at a time.
- Keep `CthughaFrameBuffer` as the buffer/palette adapter while the old globals
  still exist.
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
- PCX metadata/loading/clipping;
- `.cmd` parser and command assembly;
- `.tab` header parser and stretch behavior;
- keymap parsing with examples from `src/default.keymap`;
- deterministic flame/wave transforms after they are exposed as testable stages.
