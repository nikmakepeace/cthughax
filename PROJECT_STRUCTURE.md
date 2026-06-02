# Project Structure Map

## Top Level

```text
.
|-- src/                 application source and frontend-specific source files
|-- resources/           runtime resources loaded by the video filterchain
|   |-- map/             256-color palette maps and PNG palette previews
|   |-- img/             indexed image assets: PCX, PCX.GZ, and indexed PNG
|   `-- obj/             external 3D line object assets
|-- doc/                 original Texinfo/manual/manpage documentation
|-- tests/headers/       header/source self-containment check harness
|-- tools/               small asset-generation helpers
|-- external/minimp3/    embedded MP3 decoder used by the modern audio path
|-- external/cthugha-js/ JavaScript port/reference implementation
|-- build/               populated CMake build directory
|-- precompiled/         old 32-bit Linux binaries
|-- CMakeLists.txt       modern build entry point
|-- cmake/config.h.in    CMake config header template
`-- cthugha.ini.eg       example config file
```

`project-docs/` contains short compatibility pointers to the authoritative root
`PROJECT_*.md` files.

The tree contains local build artifacts in `src/`, `doc/`, and
`tests/headers/build/`. Some stale object files refer to code that is no longer
present, such as old server/network objects; do not treat every `.o` as
evidence of a current source module.

## Source Layout

The source tree is organized by subsystem rather than by namespace or library
boundary.

### Entrypoints

- `src/main.cc`: graphical executable entry point.
- `src/Application.*`: application lifecycle and shared per-frame scheduler.
- `src/TranslateGenerator.cc`: built-in translation-table generators and catalog
  entries.

There is no current server-mode source entry point in `src/`.

### Runtime and Composition

- `src/cthugha.h`: global platform config, logging, and timing helpers.
- `src/Settings.*`: snapshots current audio options for runtime composition.
- `src/AudioRuntime.*`: owns the active audio runtime lifecycle.
- `src/RuntimeFactory.*`: chooses audio input/output strategy from settings and
  detected environment.
- `src/PcmSourceFactory.*`: maps current settings/file names to PCM sources.
- `src/Audio.*`: modern PCM source/input/output/buffer/frame-builder classes.
- `src/AudioFrame.*`: facade exposing raw and processed 1024-sample audio
  windows to the rest of the program.
- `src/AudioVisualBridge.*`: runs audio processing, analysis, and auto-changing
  before visual mutation.
- `src/PlatformLifecycle.*`: platform pause/suspend hooks with application-owned
  suspend/resume callbacks.
- `src/VideoFilterchain.*`, `src/VideoFilterchainSequence.*`,
  `src/VideoFilterchainFactory.*`, `src/VideoFilters.*`,
  `src/VideoDirector.*`: visual-stage executor, stage ordering, filter
  composition, concrete stage filters, and visual policy.
- `src/IndexedFrame.h`: display-facing indexed frame descriptor published by
  the final video filterchain stage.
### Legacy Visual Core

- `src/CthughaBuffer.*`: single classic visual buffer dimensions and raw indexed
  active/passive pixel buffers. Per-frame flame/translate/wave/swap
  choreography lives in video filterchain filters.
- `src/CoreOption.*`, `src/CoreOptionEntry.cc`: effect registry, history,
  locks, hotkeys, and file loading helpers.
- `src/Option.*`, `src/OptionInt.cc`: scalar option classes.
- `src/AutoChanger.*`: automatic option changes based on silence, cumulative fire level,
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

Old analyzer/processor/server/network audio source files are not current source
files.

### 2D Visual Effects

- `src/flames.*`: decay/propagation feedback functions over the previous frame
  plus the global flame selection option.
- `src/Wave.*`: standalone wave domain catalog.
- `src/waves.*`: waveform, beat, object, and geometry drawing functions plus
  the global wave selection options.
- `src/sound_tables.cc`: 10 built-in wave color lookup tables.
- `src/translate.*`: translation-table loading and per-pixel remapping.
- `src/Flashlight.*`: palette-brightening visual stage.
- `src/Border.*`: hidden-row boundary stage for flame diffusion.
- `src/display.cc`: 2D screen mapping functions such as mirrored, split,
  heightfield, roll, bent, and plate.
- `src/palettes.cc`: palette loading, metadata, filtering, random palettes, and
  palette handling.
- `src/Image.*`: indexed image domain objects, placement, and image option adapter.
- `src/pcx.*`: PCX decode/encode support for indexed images.
- `src/png.*`: indexed PNG decode support for indexed images.
- `src/Screenshot.*`: screenshot filename state.

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

The wrapper files are important: several compile modes include implementation
files directly after defining a macro, for example `xwin_options.cc` defines
`CTH_XWIN` and includes `options.cc`.

## Asset Directories

### `resources/map/`

Contains 100 `.map` palettes. Format: up to 256 RGB rows, with optional
metadata lines before the RGB data. Supported metadata keys include `name`,
`set`, and `energy`. Loader: `src/palettes.cc`.

`resources/map/png/` contains 23 palette preview PNGs. These are installed by
CMake when the directory exists, but the visualizer's palette loader uses `.map`
files.

Search path:

```text
./
./resources/map/
CTH_LIBDIR/map/
--path DIR -> DIR/map/
```

### `resources/img/`

Contains the classic indexed image assets. Existing content is 6 `.pcx.gz`
files and 1 `.png` image. The image option also accepts indexed `.png` and
`.png.gz` files from the same locations. PCX loading lives in `src/pcx.cc`;
indexed PNG loading lives in `src/png.cc`. Compressed
files still go through `CoreOption::load`, which can read `.gz` by spawning
`gzip -cd`.

Search path:

```text
./
./resources/img/
CTH_LIBDIR/img/
--path DIR -> DIR/img/
```

### Translation Tables

Built-in translation tables are generated by `src/TranslateGenerator.cc` during
visual startup.

### `resources/obj/`

`src/waves.cc` can load `.obj` line objects from:

```text
./
./resources/obj/
CTH_LIBDIR/obj/
--path DIR -> DIR/obj/
```

Object format is text lines like:

```text
x1,y1,z1 - x2,y2,z2
```
