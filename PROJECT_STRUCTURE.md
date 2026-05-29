# Project Structure Map

## Top Level

```text
.
|-- src/                 application source and frontend-specific source files
|-- tab/                 translation-table generators and .cmd descriptors
|-- map/                 256-color palette maps and PNG palette previews
|-- pcx/                 PCX image assets, both plain and gzip-compressed
|-- doc/                 original Texinfo/manual/manpage documentation
|-- tests/headers/       header/source self-containment check harness
|-- tools/               small asset-generation helpers
|-- external/minimp3/    embedded MP3 decoder used by the modern audio path
|-- external/cthugha-js/ JavaScript port/reference implementation
|-- build/               populated CMake build directory
|-- precompiled/         old 32-bit Linux binaries
|-- CMakeLists.txt       modern build entry point
|-- cmake/config.h.in    CMake config header template
|-- Makefile.am          automake root
|-- configure.in         old autoconf input
|-- configure            generated configure script
|-- Makefile             generated autotools Makefile
|-- config.h             generated autotools config header
`-- cthugha.ini.eg       example config file
```

`project-docs/` contains short compatibility pointers to the authoritative root
`PROJECT_*.md` files.

The tree contains local build artifacts in `src/`, `tab/`, `doc/`, and
`tests/headers/build/`. Some stale object files refer to code that is no longer
present, such as old server/network objects; do not treat every `.o` as evidence
of a current source module.

## Source Layout

The source tree is organized by subsystem rather than by namespace or library
boundary.

### Entrypoints

- `src/initExitDisp.cc`: graphical frontend entry point and shared per-frame
  scheduler.
- `src/tabheader.cc`, `src/tabinfo.cc`: installed translation-table utilities.
- `tab/cmd_*.c`, `tab/cmdRead.cc`: translation-table generator/reader tools used
  by `.cmd` descriptors.

There is no current server-mode source entry point in `src/`.

### Runtime and Composition

- `src/cthugha.h`: global platform config, logging, timing helpers, and `run()`.
- `src/AudioRuntime.*`: owns the active audio runtime lifecycle.
- `src/RuntimeFactory.*`: chooses audio input/output strategy from settings and
  detected environment.
- `src/PcmSourceFactory.*`: maps current settings/file names to PCM sources.
- `src/Audio.*`: modern PCM source/input/output/buffer/frame-builder classes.
- `src/AudioFrame.*`: facade exposing raw and processed 1024-sample audio
  windows to the rest of the program.
- `src/AudioVisualBridge.*`: runs audio processing, analysis, and auto-changing
  before visual mutation.
- `src/VisualPipeline.*`, `src/VisualDirector.*`: visual-stage executor,
  default stage plan, and pipeline factory.
### Legacy Visual Core

- `src/CthughaBuffer.*`: classic per-buffer effect state, option instances, and
  raw indexed active/passive buffers. Per-frame flame/translate/wave/swap
  choreography now lives in visual pipeline modules.
- `src/CoreOption.*`, `src/CoreOptionEntry.cc`: effect registry, history,
  locks, hotkeys, and file loading helpers.
- `src/Option.*`, `src/OptionInt.cc`: scalar option classes.
- `src/AutoChanger.*`: automatic option changes based on silence, fire level,
  and elapsed time.
- `src/imath.*`: integer math tables/helpers used by visual code.
- `src/misc.cc`: logging helpers, time helpers, and `systemf()`.

### Audio

- `src/AudioProcessor.*`: `sound-processing` option with `none`, `Filter1`,
  `Filter2`, and `FFT`.
- `src/AudioAnalyzer.*`: RMS-like frame analysis plus rolling
  `AcousticContext` intensity/fire state.
- `src/AudioTypes.h`: shared audio sample and input-mode definitions.
- `src/AudioOptions.h`: audio option globals shared by option parsing and
  runtime construction.
- `src/AudioSystem.*`: sound lifecycle wrapper, input-mode defaults, raw-format
  options, and suspend/resume entry points.
- `src/Audio.*`: PCM sources, input, buffering, output, Pulse/OSS/null output,
  and visual-frame construction.
- `src/PcmSourceFactory.*`: maps settings and file names to line input, random
  noise, WAV, MP3, or raw PCM sources.
- `src/Mixer.*`: OSS mixer integration.
- `src/CDPlayer.*`: CD-ROM ioctl integration when compiled in.

Old analyzer/processor/server/network audio source files are not current source
files.

### 2D Visual Effects

- `src/flames.*`: decay/propagation feedback functions over the previous frame.
- `src/waves.*`: waveform, beat, object, and geometry drawing functions.
- `src/sound_tables.cc`: 10 built-in wave color lookup tables.
- `src/translate.*`: translation-table loading and per-pixel remapping.
- `src/Flashlight.*`: palette-brightening visual stage.
- `src/Border.*`: hidden-row boundary stage for flame diffusion.
- `src/display.cc`: 2D screen mapping functions such as mirrored, split,
  heightfield, roll, bent, and plate.
- `src/palettes.cc`: palette loading, metadata, filtering, random palettes, and
  palette handling.
- `src/pcx.*`: PCX loading, indexed-buffer overlay, and screenshot save.

### Display Frontends

- `src/DisplayDevice.*`: abstract display device and text rendering base.
- `src/DisplayDeviceX11.cc`: X11/Xt/Xaw display, window/root modes, MIT-SHM,
  palette setup, resize handling, and X event loop.
- `src/DisplayDeviceX11-Panel.cc`: optional Athena-widget X11 panel, including
  palette preview/metadata editing helpers.
- `src/CthughaDisplayX11.cc`: X11 display-buffer expansion, mirroring, and copy.

The current tree only wires up the X11 frontend.

### UI, Input, and Configuration

- `src/Interface.*`: interface registry, screens, selectable option rows, and
  error display.
- `src/InterfaceHelp.cc`, `src/InterfaceCredits.cc`,
  `src/InterfaceList.cc`: specific screens.
- `src/keymap.*`: configurable keymaps and action dispatch.
- `src/default.keymap`: default keymap source.
- `src/default.keymap.str`: generated C string include in in-tree builds;
  CMake generates its own copy under `build/src/`.
- `src/keys.cc`: key symbol translation and X11 key polling.
- `src/xwin_keys.cc`: X11 wrapper variant for key handling.
- `src/options.cc`: command-line and ini option handling.
- `src/xwin_options.cc`: X11 wrapper variant for option handling.
- `src/IniFiles.cc`: ini search order, wildcard matching, validation, and
  generated `.cthugha.auto`.
- `src/info_title_usage.cc`: title/help/usage output.
- `src/disp-ncurses.cc`: ncurses setup/teardown.

## Build Targets and Source Groups

### Modern CMake

Current CMake targets:

| Target | Purpose | Notes |
| --- | --- | --- |
| `xcthugha` | X11 visualizer | Built from `CTHUGHA_COMMON_SOURCES`, X11 key/options wrappers, `display.cc`, and X11 display device/display classes. |
| `tabheader` | Add/emit `.tab` headers | Built from `src/tabheader.cc`. |
| `tabinfo` | Inspect `.tab` headers | Built from `src/tabinfo.cc`. |
| `cmd_huricn`, `cmd_smoke`, `cmd_space`, `cmd_gentable`, `cmd_bighalfwheel`, `cmd_downspiral`, `cmd_randswirls`, `cmdRead` | Translation-table helpers | Built under `tab/` when `CTH_BUILD_TAB_TOOLS=ON`. |

### Autotools

`src/Makefile.am` still describes these source sets:

- `GENSRC`: shared options, audio/runtime files, UI, misc, base
  display classes, autochanger, analyzer, and pipeline scaffolding.
- `DISPSRC`: `GENSRC` plus visual effects, palettes, PCX, translation,
  `AudioProcessor`, `CthughaBuffer`, `initExitDisp`, flashlight, and help UI.

Autotools target source sets:

| Target | Purpose | Distinct source pieces |
| --- | --- | --- |
| `xcthugha` | X11 frontend | `DisplayDeviceX11.cc`, `DisplayDeviceX11-Panel.cc`, `CthughaDisplayX11.cc`, `xwin_keys.cc`, `xwin_options.cc` |
| `tabheader` | Add/emit `.tab` headers | `tabheader.cc` |
| `tabinfo` | Inspect `.tab` headers | `tabinfo.cc` |

The current generated autotools Makefiles select `xcthugha tabheader tabinfo`
and no setuid programs.

The wrapper files are important: several compile modes include implementation
files directly after defining a macro, for example `xwin_options.cc` defines
`CTH_XWIN` and includes `options.cc`.

## Asset Directories

### `map/`

Contains 100 `.map` palettes. Format: up to 256 RGB rows, with optional
metadata lines before the RGB data. Supported metadata keys include `name`,
`set`, and `energy`. Loader: `src/palettes.cc`.

`map/png/` contains 23 palette preview PNGs. These are installed by CMake when
the directory exists, but the visualizer's palette loader uses `.map` files.

Search path:

```text
./
./map/
CTH_LIBDIR/map/
--path DIR -> DIR/map/
```

### `pcx/`

Contains 12 PCX assets: 6 plain `.pcx` files and 6 `.pcx.gz` copies. Loader:
`src/pcx.cc`, through `CoreOption::load`, which can read `.gz` by spawning
`gzip -cd`.

Search path:

```text
./
./pcx/
CTH_LIBDIR/pcx/
--path DIR -> DIR/pcx/
```

### `tab/`

Contains 11 `.cmd` descriptors and 8 generator/reader source programs. `.cmd`
files are text descriptors:

```text
cmdtab
Description
command %d %d [other args]
```

The `%d %d` placeholders receive `BUFF_WIDTH` and `BUFF_HEIGHT`. Translation
generators write a table of source-pixel indexes to stdout. Loader:
`src/translate.cc`.

Search path:

```text
./
./tab/
CTH_LIBDIR/tab/WIDTHxHEIGHT/
CTH_LIBDIR/tab/
--path DIR -> DIR/tab/
```

### `obj/`

No root `obj/` asset directory is present as a maintained content directory,
but `src/waves.cc` can load `.obj` line objects from:

```text
./
./obj/
CTH_LIBDIR/obj/
--path DIR -> DIR/obj/
```

Object format is text lines like:

```text
x1,y1,z1 - x2,y2,z2
```
