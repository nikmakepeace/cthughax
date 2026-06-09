# Project Structure Map

## Top Level

```text
.
|-- src/                 application source and frontend-specific source files
|-- resources/           runtime resources loaded by the video filterchain
|   |-- map/             256-color palette maps and PNG palette previews
|   |-- img/             indexed image assets: PCX and indexed PNG
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
- `src/Configuration.*`: startup configuration sources, schema, diagnostics,
  and immutable config slices.
- `src/TranslateGenerator.cc`: built-in translation-table generators and catalog
  entries.

There is no current server-mode source entry point in `src/`.

### Runtime and Composition

- `src/cthugha.h`: global platform config, logging, and timing helpers.
- `src/AudioSettings.*`: snapshots audio config for runtime composition.
- `src/AudioIngest.*`: owns active audio acquisition, decoded history, visual
  pacing, and current frame production.
- `src/AudioPassthrough.*`: owns optional audio output passthrough and output
  cursor state.
- `src/RuntimeFactory.*`: chooses audio input/output strategy from settings and
  detected environment.
- `src/PcmSourceFactory.*`: maps current settings/file names to PCM sources.
- `src/Audio.*`: modern PCM source/input/output/buffer/frame-builder classes.
- `src/AudioFrame.*`: owned per-frame raw/processed audio data and metrics.
- `src/AudioFramePipeline.*`: runs audio processing and analysis before visual
  mutation.
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
- `src/EffectControl.*`, `src/EffectChoice.cc`: effect registry, history,
  locks, hotkeys, and file loading helpers.
- `src/Option.*`, `src/OptionInt.cc`: scalar option classes.
- `src/AutoChanger.*`: automatic option changes based on silence, cumulative
  fire level, and elapsed time. It reports quiet intervals to visual policy and
  issues automatic change commands through `RuntimeCommandSink`.
- `src/SilenceMessage.*`, `src/*MessagesProvider.*`, and
  `src/MessageFormatValidator.*`: quiet-message selection, validated
  CP437-compatible `--quiet-file`/default messages, and opt-in non-blocking
  QOTD prefetch for text injection.
- `src/imath.*`: integer math tables/helpers used by visual code.
- `src/ProcessServices.*`: process clocks, logging runtime, log sinks, and
  random-source adapters.
- `src/LegacyLoggingAdapter.cc`: temporary compatibility adapter for legacy
  `CTH_*` logging macros.

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
- `src/RuntimeFactory.*`: treats absent/failed live input as no PCM source, so
  the `AudioFrame` facade exposes silence to visual policy.
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
- `src/pcx.*`: PCX decode support for indexed images.
- `src/png.*`: indexed PNG decode support for indexed images.

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
- `src/RuntimeCommand.*`, `src/RuntimeCommandSink.h`, and
  `src/RuntimeChangeMediator.*`: live runtime command values, sink contract, and
  mediator used by keymap, interface, X11 panel, credits, audio completion, and
  AutoChanger actions before they touch
  scene/audio/display/palette-metadata/lifecycle state.
- `src/default.keymap`: default keymap source.
- `src/default.keymap.str`: generated C string include in in-tree builds;
  CMake generates its own copy under `build/src/`.
- `src/keys.cc`: key symbol translation and X11 key polling.
- `src/xwin_keys.cc`: X11 wrapper variant for key handling.
- `src/Configuration.cc`: command-line, environment, and ini startup config
  acquisition.
- `src/IniFiles.cc`: generated `.cthugha.auto` and stop-and-continue
  persistence from typed config/state snapshots.
- `src/info_title_usage.cc`: title/help/usage output.

## Build Targets and Source Groups

### Modern CMake

Current CMake targets:

| Target | Purpose | Notes |
| --- | --- | --- |
| `xcthugha` | X11 visualizer | Built from `CTHUGHA_COMMON_SOURCES`, the X11 key wrapper, `display.cc`, and X11 display device/display classes. |

The only remaining frontend wrapper in the active CMake target is
`xwin_keys.cc`, which compiles key handling with X11-specific definitions.

## Asset Directories

### `resources/map/`

Contains 100 `.map` palettes. Format: up to 256 RGB rows, with optional
metadata lines before the RGB data. Supported metadata keys include `name`,
`set`, and `energy`. Loader: `src/palettes.cc`.

The panel draws palette previews directly from the active and target `.map`
palettes; there is no separate palette-preview image directory.

Search path:

```text
./
./resources/map/
CTH_LIBDIR/map/
--path DIR -> DIR/map/
```

### `resources/img/`

Contains the classic indexed image assets. Existing content is 6 `.pcx` files
and 1 `.png` image. The image option accepts uncompressed `.pcx` and indexed
`.png` files from the same locations. PCX loading lives in `src/pcx.cc`;
indexed PNG loading lives in `src/png.cc`.

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
