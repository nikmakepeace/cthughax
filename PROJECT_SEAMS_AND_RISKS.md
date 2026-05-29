# Seams and Risks

## Best Extension Seams

### Add a Palette

Drop a `.map` file into `map/`, the current directory, installed
`CTH_LIBDIR/map/`, or `--path DIR/map/`.

Format is up to 256 RGB rows:

```text
R G B optional comment
```

The loader also accepts metadata lines before RGB data:

```text
name: Human Name
set: classic bright
energy: low medium high extreme
```

Loader: `src/palettes.cc`.

This is still the lowest-risk extension seam.

### Add or Filter a Palette Set

Palette metadata can be used with:

```sh
--palette-set SET
```

`palette_set_filter()` enables palettes whose metadata `set` contains the
requested value. If no palette matches, the code warns and leaves all palettes
enabled.

### Add a PCX Image

Drop `.pcx` or `.pcx.gz` into `pcx/`, current directory, installed
`CTH_LIBDIR/pcx/`, or `--path DIR/pcx/`.

Loader: `src/pcx.cc`.

PCX images become entries in the `pcx` CoreOption, and their palettes are added
to the palette registry. Current PCX display code centers/clips images to the
active visual buffer.

### Add a Translation Effect Without Recompiling

Add a `.cmd` descriptor and a generator program under `tab/`.

Descriptor format:

```text
cmdtab
Human-readable description
command %d %d [args]
```

The command receives `BUFF_WIDTH` and `BUFF_HEIGHT` and writes one source-pixel
index per destination pixel to stdout. `src/translate.cc` supports load-on-demand
so large tables do not need to be generated at startup.

This remains the best non-code visual effect seam.

### Add a Precomputed `.tab`

`.tab` files store a `tab_header` followed by table entries. `tabheader` and
`tabinfo` add/inspect headers.

Loader: `TranslateEntry::loaderTab` in `src/translate.cc`.

If table dimensions do not match the active buffer, stretching can be enabled.

### Add a 3D Line Object

The code supports external `.obj` line objects, though this snapshot does not
maintain a root `obj/` asset directory.

Format:

```text
x1,y1,z1 - x2,y2,z2
```

Loader: `read_object()` in `src/waves.cc`.

### Add a Compiled-In 2D Flame

Add a function to `src/flames.cc`, then add a `new FlameEntry(...)` in
`_flames`.

Contract:

- accept a `CthughaBuffer&` and read/write `activePixels()` /
  `passivePixels()`;
- respect `BUFF_WIDTH`, `BUFF_HEIGHT`, and the extra 3-line top/bottom border;
- leave coordinate remapping to the dedicated translate stage;
- expose execution through `FlameEntry::execute(buffer, context)` while
  preserving the legacy `CoreOptionEntry::operator()()` path.

### Add a Compiled-In Wave

Add a function to `src/waves.cc`, then add a `new WaveEntry(...)` in `_waves`.

Wave functions should read sound through `audioFrameProcessedData()` and rolling
state from `audioAnalysis` / `acousticContext`, then draw directly into
the buffer's active pixels. `WaveStageModule` executes the selected `WaveEntry`
through `execute(buffer, context)`.

### Add an Audio Processing Mode

Add a `CoreOptionEntry` subclass in `src/AudioProcessor.cc` and register it in
`_audioProcessorOptionEntries`.

Contract:

- read `audioFrameData()` or the active `AudioFrame`;
- write all of `audioFrameProcessedData()` or the frame's `processed` buffer;
- keep output as 1024 signed 8-bit stereo sample pairs.

### Add a PCM Source

Implement a `PcmSource` subclass in the modern audio model, then extend
`PcmSourceFactory::selectAudioSourceStrategy()` and `PcmSourceFactory::create()`.

Contract:

- publish `PcmFormat`;
- write raw PCM into the caller's buffer from `read()`;
- report finishability through `canFinish()`/`isFinished()` when applicable.

This is the preferred audio-input extension seam.

### Add an Audio Output

Implement an `AudioOutput` subclass and extend `RuntimeFactory::createAudioOutput()`.

Contract:

- implement `write()` and `isOpen()`;
- call `configureTiming()` with sample rate, bytes per sample, and chunk size;
- treat `targetDelaySamples()` as output-buffer sizing only. Visual frame
  selection is driven by the runtime visual clock, not by output latency.

### Add a Visual Pipeline Stage

Implement `VisualModule` and add it through `VisualPipelineFactory`.

Current reality: flashlight, border, indexed-buffer begin/end, image, flame,
translate, wave, and palette smoothing are explicit modules. `VisualDirector`
updates typed stage bindings before each run; flame, translate, and wave stages
execute the bound `FlameEntry`, `TranslateOption`, and `WaveEntry` objects.

The next seam to improve is the remaining selected-buffer global. Stage
execution synchronizes `CthughaBuffer::current` for UI/display compatibility,
but stage entries now receive explicit `CthughaBuffer&` objects and entry
selection no longer happens inside the stage modules.

### Add a Display Mode

2D: add a screen function in `src/display.cc` and register it in `_screens`.

GL: add a screen function in `src/GL_display.cc` and register it in that file's
`_screens`.

2D `ScreenEntry` declares an `xy size` that tells `CthughaDisplayX11/SVGA`
whether the display layer must mirror horizontally and/or vertically.

### Add a Display Frontend

Implement a new `DisplayDevice` subclass and a matching `CthughaDisplay`
subclass, or reuse the X11/SVGA-style display layer if the frontend can accept
the same 2D buffer contract.

Required seam points:

- `cth_init()`;
- `newDisplayDevice()`;
- `newCthughaDisplay()`;
- display `mainLoop()`;
- `preDraw()`, `postDraw()`, `clearBox()`, `copyBox()`, and text drawing.

This is still a large seam because many globals (`disp_size`, `bypp`,
`bytes_per_line`, `draw_mode`, `text_size`, `fontSize`) are shared.

## Hidden Couplings

### Global State Is Still The Real Architecture

Subsystems communicate mainly through globals:

- `cthughaDisplay`, `displayDevice`, `cdPlayer`, `autoChanger`;
- `audioAnalysis`, `acousticContext`;
- `BUFF_WIDTH`, `BUFF_HEIGHT`;
- `CthughaBuffer::current`;
- `screen`, `light`, `background`, `fly`;
- many `Option` and `CoreOption` singletons.

The new runtime/pipeline classes reduce some coupling, but most modules still
assume initialization order rather than checking dependencies.

### Audio Has One Runtime Model

The current audio system can use:

- `AudioRuntime`/`PcmSource`/`AudioOutput` for file playback;
- `AudioInputProcessor` for rolling live/random input.

Visual code should use `audioFrameData()` and `audioFrameProcessedData()` so
file playback, live input, random input, and silence all present the same
1024-sample frame contract.

### VisualPipeline Still Uses Legacy Selection And Binding

`VisualPipeline` now has explicit modules for image, flashlight, border,
indexed-buffer begin/end, flame, translate, wave, and palette smoothing. The
former `CthughaBuffer::run()` loop and the temporary frame-buffer binding
adapter have been removed.

Do not assume this is full inversion of control yet. UI, loading, and display
code still use `CthughaBuffer::current` to find the selected buffer.

### Build Wrappers Include `.cc` Files

Files such as `xwin_options.cc`, `svga_options.cc`, `GL_options.cc`, and
`nonx_options.cc` define a macro and include `options.cc`. Similarly,
`xwin_keys.cc`, `nonx_keys.cc`, and `GL_keys.cc` include `keys.cc`.

Any build-system rewrite must preserve separate compile units for those
wrappers, not simply compile every `.cc` file once.

### Buffer Size Affects Asset Semantics

`BUFF_WIDTH` and `BUFF_HEIGHT` affect:

- amount of sound read or framed for visuals;
- translation table dimensions;
- hidden border row pitch;
- display mirror and zoom assumptions;
- GL texture dimensions;
- table generator output.

Changing buffer size is a system-wide operation.

### Translation Is A Dedicated Stage

Translation now runs through `TranslateStageModule`. Flames should not apply
translation internally; keeping coordinate remapping in its own stage avoids the
old hidden coupling between `flames.cc` and `translate.cc`.

### Display Functions May Self-Reject

Some screen functions return nonzero when the buffer aspect ratio is unsuitable.
`CthughaDisplayX11/SVGA` calls `while (screen())` so the current display option
advances until something works.

### Palette Handling Depends on Frontend Color Mode

`palettes.cc` maintains palette entries and smoothing, but frontend display code
decides whether to use true palette hardware, pseudo-colors, or expanded
true-color lookup tables.

### Text Rendering Alters Palette/Copy Behavior

`DisplayDevice::textOnScreen`, palette darkening, `copyText`, and
`needsFullCopy` interact with palette updates and dirty copying. Text is not
just an overlay; in 8-bit modes it changes palette strategy.

## Modernization Seams

### Build System

CMake is now the verified path and should remain the reference for active work.
Autotools can be kept coherent for longtime users, but new targets should be
added to CMake first.

### Sound

OSS `/dev/dsp`, OSS mixer, and CD-ROM ioctl code remain obsolete runtime
interfaces. The best replacement seam is the modern audio composition model:

- `PcmSource` for input/decoding;
- `AudioOutput` for playback;
- `AudioFrame` for visual sampling.

Keep new audio work inside these interfaces.

### Display

X11 is the current reference frontend. SVGAlib is archival. OpenGL is retained
but depends on GLUT and old paletted-texture assumptions.

A modern display replacement should preserve the indexed 8-bit core buffer
first, then render it as a texture through SDL2/SDL3, GLFW, or similar.

### Effects

The effect code is mostly self-contained and should be preserved. The safest
path is:

1. keep `CoreOption`, `flames`, `waves`, `translate`, palettes, and PCX behavior
   stable;
2. continue moving legacy buffer binding out of stage execution and into
   explicit framebuffer/provider objects;
3. add tests around loaders and deterministic transforms before deep refactors.

### Configuration and UI

The `Option` and `CoreOption` model is workable but stringly typed. A modern UI
should treat current option entries as the domain model, then gradually wrap them
with safer metadata.

## Risk Register

### High Risk

- X11/MIT-SHM startup and image paths are platform-sensitive. A headless shell
  cannot even show `xcthugha --help` because X initialization happens first.
- Setuid root SVGAlib path remains in source; `DisplayDeviceSvga.cc` can regain
  root with `seteuid(0)` to call `vga_init()` when built that way.
- External command execution remains: `CoreOption::load()` uses `gzip -cd`,
  translation load-on-demand uses `/bin/sh -c`, and silence messages can run
  `fortune`.
- OSS and CD ioctl paths are Linux-specific, obsolete, and hard to test on
  modern systems.
- OpenGL source still assumes old GLUT/paletted-texture behavior.

### Medium Risk

- File playback has a single buffered runtime path now; the riskiest edges are
  playback latency accounting, EOF/drain behavior, and raw PCM option handling.
- `VisualPipeline` stage order is explicit now, but the stages still depend on
  global buffer selection/binding. Refactors can accidentally change which
  buffer an effect mutates.
- Many fixed-size string buffers use `sprintf`, `strncpy`, and `strncat` with
  longstanding assumptions.
- Some code assumes little-endian behavior despite partial big-endian branches.
- Palette metadata is stricter than old free-form comments; malformed metadata
  is warned/ignored before RGB data begins.
- Local build directories and object files can be stale relative to current
  source.

### Lower Risk

- Palette/map loading is relatively isolated.
- PCX loading is isolated, though format support is intentionally narrow.
- Keymap parsing is standalone and a good candidate for targeted tests.
- Translation table generators are separate command-line programs and can be
  tested independently.
- `AudioProcessor` and `AudioAnalyzer` are much easier to test than the old
  monolithic sound path.

## Places To Put Tests First

- `AudioProcessor` modes against fixed input frames.
- `AudioAnalyzer::analyze()` and `AcousticContext::update()`.
- `PcmSourceFactory::selectAudioSourceStrategy()`.
- `AudioBuffer` append/read/history behavior.
- Palette loader metadata, short files, malformed lines, and set filtering.
- PCX loading, centering, clipping, and palette behavior.
- `.cmd` parser and generated command assembly.
- `.tab` header parser and stretch behavior.
- `Keymap::parseBinding()` with `src/default.keymap` examples.
- Flame/translate/wave transforms through their current `execute()` entry-object
  paths or a small harness.
