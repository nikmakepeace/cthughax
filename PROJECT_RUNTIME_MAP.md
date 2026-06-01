# Runtime Map

## Startup Flow

Graphical frontends enter through `src/initExitDisp.cc::main`.

```text
main(argc, argv)
  srand(time(0))
  seteuid(getuid())                    # drop elevated uid
  get_pre_params()                     # early --path/--ini-file/--verbose/text options
  cth_init(&argc, argv)                # frontend-specific early init
  get_params()                         # read ini files, then command line
  title()
  init_imath()
  atexit(deleter)
  init_ncurses() if requested
  init_sound()
  new CDPlayer()
  CthughaBuffer::initAll()
  init_border()
  init_flashlight()
  newDisplayDevice()
  newCthughaDisplay()
  CoreOption::changeToInitial()
  audioProcessing.changeToInitial()
  Interface::set("main")
  Keymap::init()
  initAudioVisualBridge()              # constructs AutoChanger
  install SIGTSTP handler
  displayDevice->mainLoop()
```

There is no current server-mode startup path in source. Earlier notes about a
separate audio server or network clients are describing removed code or stale
build artifacts.

## Frame Loop

`run(int doDisplay)` in `src/initExitDisp.cc` is the shared per-frame scheduler.

```text
CthughaDisplay::nextFrame()
  publishes now/deltaT, updates FPS accounting, and enforces maxFPS

audioFrameTick()
  advances AudioRuntime and publishes the current 1024-sample audio frame/window

AudioVisualBridge::runFrame()
  applies selected sound-processing mode
  analyzes raw audio into AudioMetrics
  updates AcousticContext
  runs AutoChanger

VisualPipeline::run()
  runs visual-stage modules over the indexed visual buffers

CthughaDisplay::operator()()
  frontend-specific display composition, when doDisplay is true

CDPlayer::operator()()
  updates CD playback state

pause/suspend handling
```

The visual frame loop is cooperative. File playback now uses the modern audio
runtime; Pulse output owns its feed through its write callback, and decoded PCM
is shared with the visual engine through `AudioBuffer`/`AudioFrameBuilder`.
Translation tables are generated in-process during startup so the frame loop
receives ready tables.

### Frontend Event Loops

- X11: `DisplayDeviceX11::mainLoop()` drains Xt events, translates key releases,
  dispatches UI work, resizes to the current X window, calls `run(1)`, then runs
  interface input again.

The removed SVGAlib and OpenGL paths no longer participate in runtime control
flow.

### Pause and Resume

Graphical frontends install a `SIGTSTP` handler after initialization. Pressing
`^Z` sets `cthugha_pause`; `run()` waits until frame work is between stages,
calls `exit_sound()`, and raises `SIGTSTP`.

On `SIGCONT`, the continuation handler calls `init_sound()` and reinstalls the
stop handler before the display loop continues. This keeps suspend/resume out of
the middle of drawing code.

## Audio Pipeline

### Runtime Selection

`audioRuntimeInit()` builds the active audio runtime through `RuntimeFactory`.
Audio startup uses current options (`sound-device-number`, `sound-method`,
`silent`, `--pulse-server`, and the `--play` file name). Environment detection
checks OSS read and write availability and whether Pulse support was compiled
in.

`PcmSourceFactory` currently selects:

- `ASS_LineIn` for `AIM_DSPIn`;
- `ASS_Random` for `AIM_Random`;
- `ASS_WavFile` for `--play *.wav`;
- `ASS_Mp3File` for `--play *.mp3`;
- `ASS_RawFile` for other `--play` names, using `sound-format`,
  `sound-channels`, and `sound-sample-rate`;
- `ASS_Unknown` for unsupported cases.

### Native File Pipeline

For WAV, MP3, and raw PCM file playback, `AudioRuntime` uses:

```text
PcmSource -> AudioInput -> AudioBuffer -> AudioOutput
                         -> AudioFrameBuilder -> AudioFrame
```

Important pieces:

- `WavPcmSource` parses and reads WAV data.
- `Minimp3PcmSource` decodes MP3 data through `external/minimp3`.
- `RawPcmSource` reads headerless PCM according to the current raw-audio
  format options.
- `AudioOutput` is selected as Pulse, OSS DSP output, or null output. Pulse
  connects to `--pulse-server` / `cthugha.pulse-server` when set.
- `AudioBuffer` stores decoded/submitted sample history.
- `AudioFrameBuilder` builds the 1024-sample visual frame around the currently
  audible sample, with compensation for observed visual latency.

When playback reaches the end and the output has drained, `AudioRuntime` sets
completion and requests program close.

### Native Input Processor Path

For live OSS input and random noise, `RuntimeFactory` can create:

```text
PcmSource -> AudioInput -> AudioInputProcessor
```

`AudioInputProcessor` keeps the rolling 1024-sample normalized internal frame
and processed workspace used by visual code.

### Input Fallbacks

There is no separate old backend fallback path. Live input uses the same modern
`PcmSource`/`AudioInputProcessor` model as random input, and file playback uses
the buffered `AudioInput` -> `AudioBuffer` -> `AudioOutput` path. If line input
cannot be opened, startup falls back to `RandomNoisePcmSource` unless sound is
explicitly disabled.

### AudioFrame Facade

Visual and UI code should use the facade in `src/AudioFrame.cc`:

```text
audioFrameRawData()
audioFrameProcessedWaveData()
audioFrameCurrent()
audioFrameTick()
audioFrameChange()
```

These functions choose the current source in this order:

1. `AudioRuntime` frame from the native file pipeline;
2. `AudioInputProcessor` frame from the native input path;
3. silent static buffers.

### Sound Processing

The `sound-processing` option is implemented by `src/AudioProcessor.cc`.

Built-in entries:

- `none`: copy raw audio to processed wave data.
- `Filter1`: slope-limit sharp sample jumps.
- `Filter2`: low-pass-ish smoothing.
- `FFT`: custom 1024-sample FFT using left/right channels as real/imaginary
  components.

`AudioVisualBridge::runFrame()` calls `audioProcessing.process()` before
analysis and before visual mutation. Waves normally read
`audioFrameProcessedWaveData()`.

### Analysis and Acoustic Context

`AudioAnalyzer::operator()()` computes frame-local `AudioMetrics`:

- `amplitude`;
- `amplitudeLeft`;
- `amplitudeRight`;
- `noisy`, based on `min-noise`.

`AcousticContext` then maintains rolling state:

- `intensity()`: smoothed normalized amplitude;
- `fire()`: emitted when a rising attack ends;
- `cumulativeFireLevel()`: accumulated fire used by `AutoChanger`.

`AutoChanger` uses `audioMetrics.noisy` for silence handling and
`acousticContext.cumulativeFireLevel()` for fire-driven option changes.

## Visual Buffer Pipeline

`CthughaBuffer` owns the single classic visual buffer dimensions and the raw
active/passive indexed pixel buffers. It does not own per-frame
flame/translate/wave choreography or the current frame palette; `VisualDirector`
configures pipeline modules with the currently selected effect entries. Default
dimensions are:

```text
BUFF_WIDTH  = 160
BUFF_HEIGHT = 100
```

Each allocation has three hidden rows above and below the visible buffer. Flame
code reads those rows as boundary data.

### VisualPipeline Order

`VisualDirector::defaultPipelineSequence()` currently includes these stages:

```text
ImageStage
BorderStage
FlameStage
TranslateStage
WaveStage
FrameCommitStage
PaletteStage
FlashlightStage
```

`VisualPipelineFactory::create()` in `src/VisualPipelineFactory.cc` currently
expands that sequence into this module order:

```text
ImageStageModule
BorderVisualModule
FlameStageModule
TranslateStageModule
WaveStageModule
FrameCommitModule
PaletteStageModule
FlashlightVisualModule
```

`ImageStageModule`, `FlameStageModule`, `TranslateStageModule`, and
`WaveStageModule` are real stages. Image overlays the current `IndexedImage`
when `VisualDirector` arms the one-shot image stage. PCX and indexed PNG files
are decoded into that domain object before the frame loop. Before each frame,
`VisualDirector` updates the stage modules with the selected image, selected
flame, general-flame value, prepared translation object, wave, and border mode.
`VisualPipeline::run()` then wraps the current buffer, frame context, and
display palette in a `VisualFrame` and passes that frame through each enabled
stage.

Image and translate selection flow through `VisualDirector`.

### Flashlight

`apply_flashlight()` is a palette effect. If the `flashlight` CoreOption is on,
`VisualDirector` enables the flashlight stage. The stage copies the current
`FramePalette`, brightens low palette entries according to
`acousticContext.fire()`, and writes the temporary palette back to
`FramePalette`.

It does not draw pixels into the indexed buffer.

### Border

`apply_border()` fills the three hidden rows above and below the active buffer.

The selected `border` CoreOption decides whether those rows are:

- zero;
- copied from current audio data;
- filled with current amplitude;
- filled with 255.

### Indexed Buffer Stages

The indexed-buffer mutation path is split across visual pipeline modules. The
frame-level order is:

```text
ImageStageModule
  overlay the selected IndexedImage when VisualDirector has armed ImageStage once

FlameStageModule
  execute the selected Flame against the frame buffer

TranslateStageModule
  execute the bound Translate object with its ready table

WaveStageModule
  execute the selected Wave against the frame buffer

FrameCommitModule
  log a limited visual-buffer summary
  swap(activeBuffer, passiveBuffer)
```

The order matters:

- `flame` propagates/decays the previous finished image.
- `translate` remaps pixels using loaded translation tables as its own stage.
- `wave` draws fresh sound-reactive marks into the active buffer.
- swapping makes the finished frame become `passiveBuffer`, which display code
  reads.

Palette smoothing is separate from the indexed pixel mutation stages.

### Palette Stage

`PaletteOption` is the global CoreOption adapter for loaded palettes.
`PaletteEntry` wraps a `ColorPalette` plus UI/config metadata, while
`VisualDirector` binds the selected entry into `PaletteStageModule`.
`PaletteStageModule` delegates transition mechanics to `PaletteTransition`,
which moves the output palette toward the target `ColorPalette` over a frame
budget.
The resulting palette and dirty flag live in `FramePalette`, which the display
device reads when applying the hardware/X11 palette for the frame.
`PaletteTransition` keeps its own unflashed transition state so flashlight can
overlay the final frame palette without becoming the starting point for the
next smoothing step.
The global `paletteSmoothingChance` controls whether a palette change smooths
or jumps directly to the new palette.
When smoothing is used, `VisualDirector` asks `PaletteTransition` to use a
random named strategy: RGB linear, RGB squared, or HSL interpolation.

Command-line options:

- `--palette-smoothing VALUE`, clamped to `0.0..1.0`;
- `--no-palette-smoothing`;
- `--palette-set SET`, which filters palettes by metadata set.

`FlashlightVisualModule` runs after palette smoothing so it brightens the final
palette output for the frame instead of being diluted by the smoothing step.

## CoreOption System

`CoreOption` is the runtime effect registry. It stores:

- linked-list membership for `changeOne()`/`changeAll()`;
- current value;
- initial entry from config/CLI;
- per-entry `use` flag;
- per-option lock;
- history stack for restore;
- 10 hotkey values.

`CoreOptionEntry` is callable via `operator()()`. Subclasses wrap functions,
loaded assets, display functions, or no-op entries.

Initial values come from ini files and command-line arguments. Startup applies
them with:

```text
CoreOption::changeToInitial()
audioProcessing.changeToInitial()
```

## Display Pipeline

The X11 display path is:

```text
displayDevice->setGlobalPalette()
displayDevice->preDraw()
choose direct or temporary indexed buffer
checkZoom()
screen() maps selected passive pixels into CthughaDisplay::buffer
optional horizontal mirror
palette expansion if target is not 8-bit indexed
optional vertical mirror
clear border
zoom2Screen()
displayDevice->prePrint()
Interface::display()
errors.display()
displayDevice->postPrint()
displayDevice->postDraw()
```

The display function selected by the `display` CoreOption comes from
`src/display.cc`. It maps one `BUFF_WIDTH x BUFF_HEIGHT`
passive buffer into an image that is commonly mirrored to
`2 * BUFF_WIDTH x 2 * BUFF_HEIGHT`.

## User Input and Interface

The UI is not a separate event framework. Each frontend polls or receives keys,
then calls `Interface::current->run()`. `Interface` dispatches keys through
`Keymap`.

Default mappings come from `src/default.keymap`, transformed into
`default.keymap.str` by the active build.

Keymap contexts include:

- `default`;
- `main`;
- `Help`;
- `CD`;
- `sound`;
- `Options`;
- `CoreOptions`;
- `OptionElement`;
- `CoreOptionElement`;
- `playList`;
- `border`;
- `flashlight`.

Actions are registered by static `ACTION(name)` objects in `keymap.cc`,
`Interface.cc`, `InterfaceHelp.cc`, `InterfaceList.cc`, `CDPlayer.cc`, and
related modules.

## Configuration Flow

`get_pre_params()` handles only early options such as `--path`, `--ini-file`,
`--verbose`, and X11 terminal-text options. `get_params()` then reads ini files
and reprocesses the full command line, so command-line options override ini
values.

Without `--ini-file`, `read_ini()` searches:

1. `CTH_LIBDIR/cthugha.ini`
2. `~/.cthugha.auto`
3. `~/.cthugha.ini`
4. `./cthugha.ini`
5. `--path DIR` -> `DIR/cthugha.ini`
6. X11 resource database for `xcthugha`, via `open_ini_sys()`

With `--ini-file FILE`, only that file is opened. Ini chaining from inside an
ini file is intentionally ignored with a warning.

Entry format is:

```text
cthugha.option: value
cthugha.feature.buffer: value
cthugha.feature.buffer.entry: on/off
```

The ini reader supports `?` wildcards in entry names. Pressing `a` at runtime
can write `~/.cthugha.auto` when saving is enabled.
