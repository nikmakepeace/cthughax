# Seams and Risks

## Best Extension Seams

### Add a Palette

Drop a `.map` file into `resources/map/`, the current directory, installed
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

### Add an Indexed Image

Drop uncompressed `.pcx` or indexed `.png` files into `resources/img/`, current
directory, installed `CTH_LIBDIR/img/`, or `--path DIR/img/`.

Loaders: `src/pcx.cc` and `src/png.cc`.

Image files become entries in the image option owned by `VideoDirector`.
PCX/PNG source palettes are retained with the image entry for future policy, but
image display does not mutate the current frame palette. `ImageFilter`
clips the selected `IndexedImage` into the active visual buffer when armed.

### Add a Translation Effect

Add a `TranslateGenerator` implementation and register it in
`defaultTranslationCatalog()` in `src/TranslateGenerator.cc`. A catalog entry
pairs an id, a display description, a generator, and per-entry options, so the
same generator can appear multiple times with different names and parameters.

`src/TranslationOptions.cc` eagerly generates these tables during visual startup so the
running filterchain only executes ready maps.

### Add a 3D Line Object

External `.obj` line objects live under `resources/obj/`.

Format:

```text
x1,y1,z1 - x2,y2,z2
```

Loader: `read_object()` in `src/waves.cc`.

### Add a Compiled-In 2D Flame

Add a function to `src/flames.cc`, then add a `Flame(...)` in
`src/Flame.cc::flameCatalog`. Also add a matching `FlameEntry` adapter in
`src/flames.cc::_flames` so the `FlameOption` can expose it to keymap/UI/config
selection.

Contract:

- accept a `CthughaBuffer&` and read/write `activePixels()` /
  `passivePixels()`;
- respect `BUFF_WIDTH`, `BUFF_HEIGHT`, and the extra 3-line top/bottom border;
- leave coordinate remapping to the dedicated translate stage;
- expose execution through the `Flame` domain object, not through
  `EffectChoice`.

### Add a Compiled-In Wave

Add a drawing function to `src/waves.cc`, declare it in `src/Wave.cc`, and add
it to `waveCatalog`. Add or adjust the corresponding `WaveEntry` adapter only
if the UI/EffectControl list needs a different in-use flag.

Wave functions should read sound through `audioFrameProcessedWaveData()` and rolling
state from `audioMetrics` / `acousticContext`, read selected scale/table/object
values from `WaveRuntime`, then draw directly into the buffer's active pixels.
`WaveFilter` executes the selected `Wave` through
`execute(buffer, context, runtime)`.

### Add an Audio Processing Mode

Add a `EffectChoice` subclass in `src/AudioProcessor.cc` and register it in
`_audioProcessorOptionEntries`.

Contract:

- read `audioFrameRawData()` or the active `AudioFrame`;
- write all of `audioFrameProcessedWaveData()` or the frame's `processedWaveData` buffer;
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

### Add a Video Filter

Implement `VideoFilter` and add it through `VideoFilterchainFactory`.

Current reality: image, border, flame, translate, wave, text injection,
frame-commit, palette smoothing, flashlight, and indexed-frame export are
explicit filters. `VideoDirector` updates typed stage objects before each run;
`FlameFilter` owns the current `Flame` and general-flame value.
`VideoDirector` chooses a runnable `Wave`, configures it with wave
scale/table/object, and binds only that `Wave` into `WaveFilter`. The
filterchain passes a `VideoFrame` through each stage;
that frame carries the current `CthughaBuffer`, frame context, display palette,
and `IndexedFrame` publication slot.

The next seam to improve is the remaining display compatibility layer. Stage
entries receive an explicit `VideoFrame` and entry selection does not happen
inside the stage filters. Display presentation now receives an explicit
`IndexedFrame`, but scratch allocation and backend globals still assume the
classic runtime shape.

### Add a Display Mode

2D: add a screen function in `src/display.cc` and register it in `_screens`.

`ScreenEntry` declares an `xy size` that tells `CthughaDisplayX11`
whether the display layer must mirror horizontally and/or vertically.

### Add a Display Frontend

Implement a new `DisplayDevice` subclass and a matching `CthughaDisplay`
subclass, or reuse the X11-style display layer if the frontend can accept the
same 2D buffer contract.

Required seam points:

- `cth_init()`;
- `newDisplayDevice()`;
- `newCthughaDisplay()`;
- display `processEvents()`;
- `preDraw()`, `postDraw()`, `clearBox()`, `copyBox()`, and text drawing.

This is still a large seam because many globals (`disp_size`, `bypp`,
`bytes_per_line`, `draw_mode`, `text_size`, `fontSize`) are shared.

## Hidden Couplings

### Global State Is Still The Real Architecture

Subsystems communicate mainly through globals:

- `cthughaDisplay`, `displayDevice`, `autoChanger`;
- `audioMetrics`, `acousticContext`;
- `BUFF_WIDTH`, `BUFF_HEIGHT`;
- `CthughaBuffer::current`;
- `screen`;
- many `Option` and `EffectControl` singletons.

The new runtime/filterchain classes reduce some coupling, but most filters still
assume initialization order rather than checking dependencies.

### Audio Has One Runtime Model

The current audio system can use:

- `AudioRuntime`/`PcmSource`/`AudioOutput` for file playback;
- `AudioInputProcessor` for rolling live/random input.

Visual code should use `audioFrameRawData()` and `audioFrameProcessedWaveData()` so
file playback, live input, random input, and silence all present the same
1024-sample frame contract.

### Display Presentation Still Shares Backend Globals

`VideoFilterchain` has explicit filters for image, border, flame, translate,
wave, text injection, frame commit, palette smoothing, flashlight, and
indexed-frame export. It
creates one `VideoFrame` from the current buffer, context, display palette, and
`IndexedFrame` publication slot, then passes that frame through enabled filters
in stage order.

Do not assume this is full inversion of control yet. The display path consumes
`IndexedFrame`, but X11-era globals still own backend memory layout, frame
scratch allocation, and event-loop handoff.

### Frontend Key Wrapper Includes `.cc`

`xwin_keys.cc` includes `keys.cc` with X11-specific definitions. Any
build-system rewrite must preserve that compile unit until key handling no
longer needs a macro-specialized wrapper.

### Buffer Size Affects Asset Semantics

`BUFF_WIDTH` and `BUFF_HEIGHT` affect:

- amount of sound read or framed for visuals;
- translation table dimensions;
- hidden border row pitch;
- display mirror and zoom assumptions;
- table generator output.

Changing buffer size is a system-wide operation.

### Translation Is A Dedicated Stage

Translation runs through `TranslateFilter`. Flames should not apply
translation internally; coordinate remapping belongs in its own stage rather
than inside `flames.cc`.

### Display Functions May Self-Reject

Some screen functions return nonzero when the buffer aspect ratio is unsuitable.
That is only a render rejection, not a request to change the selected display
option. `PresentationComposer` handles fallback frame-locally by trying the last
successfully rendered screen and then the safe `Up` screen.

### Palette Handling Depends on Frontend Color Mode

`palettes.cc` maintains the `PaletteOption` adapter and loaded `PaletteEntry`
objects. `ColorPalette`, `PaletteTransition`, and `FramePalette` own palette
data, smoothing, and dirty display output, but frontend display code still
decides whether to use true palette hardware, pseudo-colors, or expanded
true-color lookup tables.

### Text Rendering Alters Palette/Copy Behavior

`DisplayDevice::textOnScreen`, palette darkening, `copyText`, and
`needsFullCopy` interact with palette updates and dirty copying. Text is not
just an overlay; in 8-bit modes it changes palette strategy.

## Modernization Seams

### Build System

CMake is now the verified path and should remain the reference for active work.
The old autotools path has been removed; add new targets to CMake.

### Sound

OSS `/dev/dsp` and OSS mixer code remain obsolete runtime interfaces. The best
replacement seam is the modern audio composition model:

- `PcmSource` for input/decoding;
- `AudioOutput` for playback;
- `AudioFrame` for visual sampling.

Keep new audio work inside these interfaces.

### Display

X11 is the current reference frontend. The old SVGAlib and OpenGL paths have
been removed.

A modern display replacement should preserve the indexed 8-bit core buffer
first, then render it as a texture through SDL2/SDL3, GLFW, or similar.

### Effects

The effect code is mostly self-contained and should be preserved. The safest
path is:

1. keep `EffectControl`, `flames`, `waves`, `translate`, palettes, and indexed image behavior
   stable;
2. continue moving `CthughaBuffer::current` lookups behind explicit
   display/provider objects;
3. add tests around loaders and deterministic transforms before deep changes.

### Configuration and UI

The `Option` and `EffectControl` model is workable but stringly typed. A modern UI
should treat current option entries as the domain model, then gradually wrap them
with safer metadata.

`RuntimeCommandSink` / `RuntimeChangeMediator` is the current small
runtime-change seam. Keymap/interface code, X11 panel callbacks, and
AutoChanger issue typed `RuntimeCommand` values through the sink; credits input
and file-playback completion use it for close requests. X11 palette metadata
save/revert callbacks use it for panel-local persistence and editor state. The
mediator delegates to existing owners, handles save-and-continue persistence,
and returns a `RuntimeChangeSet`. It is intentionally a boundary around today's
globals rather than a complete ownership rewrite.

## Risk Register

### High Risk

- X11/MIT-SHM startup and image paths are platform-sensitive, though X11
  initialization is now deferred until display startup and command-line help
  exits before X is touched.
- Silence messages no longer execute `fortune`; opt-in QOTD input is public
  network text and must stay strictly validated and bounded.
- OSS audio and mixer paths are Linux-specific, obsolete, and hard to test on
  modern systems.

### Medium Risk

- File playback has a single buffered runtime path; the riskiest edges are
  playback latency accounting, EOF/drain behavior, and raw PCM option handling.
- `VideoFilterchain` stage order is explicit, and stage execution uses
  director-provided buffers. The remaining risk is non-filterchain code that still
  consults `CthughaBuffer::current`.
- Many fixed-size string buffers use `sprintf`, `strncpy`, and `strncat` with
  longstanding assumptions.
- Some code assumes little-endian behavior despite partial big-endian branches.
- Palette metadata is stricter than old free-form comments; malformed metadata
  is warned/ignored before RGB data begins.
- Local build directories and object files can be stale relative to current
  source.

### Lower Risk

- Palette/map loading is relatively isolated.
- Indexed image loading is isolated. PNG support is intentionally limited to
  non-interlaced indexed-color PNGs.
- Keymap parsing is standalone and a good candidate for targeted tests.
- Translation table generators are in-process domain objects and can be
  tested without subprocess plumbing.
- `AudioProcessor` and `AudioAnalyzer` are much easier to test than the old
  monolithic sound path.

## Places To Put Tests First

- `AudioProcessor` modes against fixed input frames.
- `AudioAnalyzer::analyze()` and `AcousticContext::update()`.
- `PcmSourceFactory::selectAudioSourceStrategy()`.
- `AudioBuffer` append/read/history behavior.
- Palette loader metadata, short files, malformed lines, and set filtering.
- Indexed image loading, placement, clipping, and source-palette retention.
- built-in translation generator catalog entries and deterministic seeds.
- `Keymap::parseBinding()` with `src/default.keymap` examples.
- Flame, translate, and wave transforms through their domain-object `execute()`
  paths, or all three through a small harness.
