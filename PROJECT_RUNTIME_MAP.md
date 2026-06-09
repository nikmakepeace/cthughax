# Runtime Map

## Startup Flow

Graphical frontends enter through `src/main.cc::main`.

```text
main(argc, argv)
  application = new Application(argc, argv)
  if application->initialize()
    application->run()
```

`Application::initialize()` does the startup work:

```text
  seteuid(getuid())                    # drop elevated uid
  buildStartupConfig(argc, argv)       # defaults, ini files, env, command line
  loggingRuntimeValue.configure(LoggingConfig)
  emit diagnostics; handle failure/help before display work
  configureKeys(InputConfig)
  configureAudioOptions(AudioConfig)
  configureCthughaDisplay(DisplayConfig)
  configureDisplayDeviceX11(X11Config)
  configureAutoChanger(AutoChangeConfig)
  configure effect/message/transition policies
  CthughaBuffer::buffer.setDimensions(DisplayConfig)
  remove_continuation_ini(PathConfig)
  initSceneRuntime()
  title()
  videoDirector().silenceMessages().initialize()
  init_imath()
  init_sound(AudioConfig, RuntimeCommandSink)
  initializeVisualCatalogs(PathConfig, RandomSource)
  CthughaBuffer::buffer.allocatePixels()
  loadEffectPolicyImages(EffectPolicy, PathConfig)
  init_border()
  init_flashlight()
  sceneCommands().applyStartupConfig(SceneConfig)
  configureAudioProcessing(SceneConfig)
  Interface::set("main")
  Keymap::init(InputConfig)
  cth_init(&argc, argv)                # frontend-specific display init
  newDisplayDevice(DisplayConfig)
  newCthughaDisplay()
  initAudioFramePipeline()             # constructs audio pipeline and Application-owned AutoChanger
  PlatformLifecycle::install()         # platform pause/suspend hook
```

`Application::run()` owns the runtime loop:

```text
while !runtimeShutdown.closeRequested()
  displayDevice->processEvents()
  Interface::current->run()
  runFrame(1)
  Interface::current->run()
```

There is no current server-mode startup path in source. Earlier notes about a
separate audio server or network clients are describing removed code or stale
build artifacts.

## Frame Loop

`Application::runFrame(int doDisplay)` in `src/Application.cc` is the shared
per-frame scheduler.

```text
CthughaDisplay::nextFrame()
  publishes now/deltaT, updates FPS accounting, and enforces maxFPS

audioFrameTick()
  advances AudioRuntime and publishes the current 1024-sample audio frame/window

AudioFramePipeline::processFrame()
  applies selected sound-processing mode
  analyzes raw audio into AudioMetrics
  updates AcousticContext

Application::runAudioFramePipeline()
  runs AutoChanger against the analyzed frame metrics

VideoFilterchain::run()
  runs visual-stage filters and publishes IndexedFrame

CthughaDisplay::present(IndexedFrame)
  frontend-specific display composition, when doDisplay is true and the
  filterchain produced a valid indexed frame

pause/suspend handling
```

The visual frame loop is cooperative. File playback now uses the modern audio
runtime; Pulse output owns its feed through its write callback, and decoded PCM
is shared with the visual engine through `AudioBuffer`/`AudioFrameBuilder`.
Translation tables are generated in-process during startup with the
Application-owned `RandomSource` where generated tables request randomized
seeds, so the frame loop receives ready tables.

### Frontend Event Loops

- X11: `DisplayDeviceX11::processEvents()` drains pending Xt events, translates
  key releases, dispatches toolkit events, and resizes to the current X window.
  It does not advance audio/video; `Application::run()` does that after event
  processing.

The removed SVGAlib and OpenGL paths no longer participate in runtime control
flow.

### Pause and Resume

`PlatformLifecycle` owns platform suspend requests.  On POSIX systems it
installs a `SIGTSTP` handler with `sigaction`; that handler only records a
pending suspend request.

`Application::runFrame()` services lifecycle requests only at the frame boundary.
It calls application-owned callbacks to stop audio before the process suspends
and restart audio after it resumes.  Platforms without POSIX job control get a
no-op lifecycle hook that future native ports can replace.

## Audio Pipeline

### Runtime Selection

`audioRuntimeInit()` builds the active audio runtime through `RuntimeFactory`.
Audio startup uses an `AudioConfig` slice containing source mode, file name,
sample format, output policy, latency, device, and mixer choices. Environment
detection checks OSS read and write availability and whether Pulse support was
compiled in.

`PcmSourceFactory` currently selects:

- `ASS_LineIn` for `AIM_DSPIn`;
- `ASS_Random` for `AIM_Random`;
- `ASS_WavFile` for `--play *.wav`;
- `ASS_Mp3File` for `--play *.mp3`;
- `ASS_RawFile` for other `--play` names, using `sound-format`,
  `sound-channels`, and `sound-sample-rate`;
- `ASS_Unknown` for `AIM_None` and unsupported cases.

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
completion and issues `RuntimeCommand::requestClose()` through the configured
runtime command sink.

### Native Input Processor Path

For live OSS input and explicit random-noise input, `RuntimeFactory` can create:

```text
PcmSource -> AudioInput -> AudioInputProcessor
```

`AudioInputProcessor` keeps the rolling 1024-sample normalized internal frame
and processed workspace used by visual code.

### Input Fallbacks

There is no separate old backend fallback path. Live input uses the same modern
`PcmSource`/`AudioInputProcessor` model as random input, and file playback uses
the buffered `AudioInput` -> `AudioBuffer` -> `AudioOutput` path. If no input is
requested, or line input is unavailable/cannot be opened, the runtime has no PCM
source; the `AudioFrame` facade publishes its static silent buffers to visual
policy.

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

`Application::runAudioFramePipeline()` calls the owned audio frame pipeline
before visual mutation. Waves normally read the processed wave data through
the current screen render context.

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
`acousticContext.cumulativeFireLevel()` for fire-driven option changes. When it
decides to change effects, it issues either `RuntimeCommand::changeOne()` or
`RuntimeCommand::changeAll()` through `RuntimeCommandSink`; the mediator then
delegates concrete selection to `SceneCommands` and `EffectControl`.

## Video Buffer Filterchain

`CthughaBuffer` owns the single classic visual buffer dimensions and the raw
active/passive indexed pixel buffers. It does not own per-frame
flame/translate/wave choreography or the current frame palette; `VideoDirector`
configures filterchain filters with the currently selected effect entries. Default
dimensions are:

```text
BUFF_WIDTH  = 160
BUFF_HEIGHT = 100
```

Each allocation has three hidden rows above and below the visible buffer. Flame
code reads those rows as boundary data.

### VideoFilterchain Order

`VideoDirector::defaultFilterchainSequence()` currently includes these stages:

```text
ImageStage
BorderStage
FlameStage
TranslateStage
WaveStage
TextStage
FrameCommitStage
PaletteStage
FlashlightStage
IndexedFrameStage
```

`VideoFilterchainFactory::create()` in `src/VideoFilterchainFactory.cc` currently
expands that sequence into this filter order:

```text
ImageFilter
BorderFilter
FlameFilter
TranslateFilter
WaveFilter
TextInjectionFilter
FrameCommitFilter
PaletteFilter
FlashlightFilter
IndexedFrameFilter
```

`ImageFilter`, `FlameFilter`, `TranslateFilter`, `WaveFilter`, and
`TextInjectionFilter` are real pixel-mutating stages. Image overlays the current
`IndexedImage` when `VideoDirector` arms the one-shot image stage; placement is
chosen with the Application-owned `RandomSource`. Text
injection stamps wrapped CP437 text into active pixels when `VideoDirector` arms
the text stage. Quiet-message text is selected by `SilenceMessage` from
validated `--quiet-file` messages, opt-in QOTD prefetches, or defaults using
the Application-owned `RandomSource`; QOTD socket prefetches also receive an
Application-owned countdown timer factory, routed through `SceneCueInjectText`,
then observed by `VideoDirector` to arm `TextInjectionFilter`. PCX and indexed
PNG files are decoded into that domain object before the frame loop. Before
each frame, `VideoDirector` updates the stage filters with the selected image,
selected flame, general-flame value, prepared translation object, wave, and
border mode.
General-flame and generic EffectControl randomization are commanded through
`SceneCommands` and use the Application-owned `RandomSource`.
`VideoFilterchain::run()` then wraps the current buffer, frame context, display
palette, and indexed-frame publication slot in a `VideoFrame`, then passes that
frame through each enabled stage.

Image and translate selection flow through `VideoDirector`.

### Flashlight

`apply_flashlight()` is a palette effect. If the `flashlight` EffectControl is on,
`VideoDirector` enables the flashlight stage. The stage copies the current
`FramePalette`, brightens low palette entries according to
`acousticContext.fire()`, and writes the temporary palette back to
`FramePalette`.

It does not draw pixels into the indexed buffer.

### Border

`apply_border()` fills the three hidden rows above and below the active buffer.

The selected `border` EffectControl decides whether those rows are:

- zero;
- copied from current audio data;
- filled with current amplitude;
- filled with 255.

### Indexed Buffer Stages

The indexed-buffer mutation path is split across video filterchain filters. The
frame-level order is:

```text
ImageFilter
  overlay the selected IndexedImage when VideoDirector has armed ImageStage once

FlameFilter
  execute the selected Flame against the frame buffer

TranslateFilter
  execute the bound Translate object with its ready table

WaveFilter
  execute the selected Wave against the frame buffer

FrameCommitFilter
  log a limited visual-buffer summary
  swap(activeBuffer, passiveBuffer)
```

The order matters:

- `flame` propagates/decays the previous finished image.
- `translate` remaps pixels using loaded translation tables as its own stage.
- `wave` draws fresh sound-reactive marks into the active buffer.
- swapping makes the finished frame become `passiveBuffer`, which display code
  reads.

Wave renderers receive the Application-owned `RandomSource` through
`VideoDirector` -> `WaveFilter` -> `WaveRuntime`; random object axes, firework
placement, warp rings, and random stick lines do not use process-global random
state.

Palette smoothing is separate from the indexed pixel mutation stages.

### Palette Stage

`PaletteOption` is the global EffectControl adapter for loaded palettes.
`PaletteEntry` wraps a `ColorPalette` plus UI/config metadata, while
`VideoDirector` binds the selected entry into `PaletteFilter`.
Randomly generated palettes are created by `PaletteRandomGenerator` with the
Application-owned `RandomSource`, routed through `SceneCommands`.
`PaletteFilter` delegates transition mechanics to `PaletteTransition`,
which moves the output palette toward the target `ColorPalette` over a frame
budget.
The resulting palette and dirty flag live in `FramePalette`, which the display
device reads when applying the hardware/X11 palette for the frame.
`PaletteTransition` keeps its own unflashed transition state so flashlight can
overlay the final frame palette without becoming the starting point for the
next smoothing step.
The global `paletteSmoothingChance` controls whether a palette change smooths
or jumps directly to the new palette.
When smoothing is used, `VideoDirector` uses the Application-owned
`RandomSource` to choose the `PaletteTransition` strategy: RGB linear,
RGB squared, or HSL interpolation.

Command-line options:

- `--palette-smoothing VALUE`, clamped to `0.0..1.0`;
- `--no-palette-smoothing`;
- `--palette-set SET`, which filters palettes by metadata set.

`FlashlightFilter` runs after palette smoothing so it brightens the final
palette output for the frame instead of being diluted by the smoothing step.

## EffectControl System

`EffectControl` is the runtime effect registry. It stores:

- linked-list membership for `changeOne()`/`changeAll()`;
- current value;
- initial entry from config/CLI;
- per-entry `use` flag;
- per-option lock;
- history stack for restore;
- 10 hotkey values.

`EffectChoice` is callable via `operator()()`. Subclasses wrap functions,
loaded assets, display functions, or no-op entries.

Initial values come from ini files and command-line arguments. Startup scene
choices are applied by `SceneCommands::applyStartupConfig(SceneConfig)`, which
passes the Application-owned `RandomSource` into `EffectControl::change(...)`
for empty or invalid random fallbacks.

Generic random changes are also routed through `SceneCommands`:

```text
EffectControl::changeOne(RandomSource&)
EffectControl::changeAll(RandomSource&)
```

## Display Pipeline

The X11 display path is:

```text
displayDevice->setGlobalPalette()
displayDevice->preDraw()
choose direct or temporary indexed buffer
checkZoom()
PresentationComposer maps selected IndexedFrame pixels into CthughaDisplay::buffer
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

`PresentationComposer` treats the `screen` option as requested user intent. If
the requested screen renderer rejects the current source geometry, the composer
falls back for that frame to the last successfully rendered screen, then to the
safe `Up` screen. It does not mutate the selected `screen` option.

The display function selected by the `display` EffectControl comes from
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
- `sound`;
- `Options`;
- `EffectControls`;
- `OptionElement`;
- `EffectControlElement`;
- `playList`;
- `border`;
- `flashlight`.

Actions are registered by static `ACTION(name)` objects in `keymap.cc`,
`Interface.cc`, `InterfaceHelp.cc`, `InterfaceList.cc`, and related modules.
Keymap, generic interface mutation actions, X11 panel selection/quit/palette
metadata callbacks, credits key handling, playback completion, and AutoChanger
now issue `RuntimeCommand` values through a `RuntimeCommandSink`.
`RuntimeChangeMediator` implements that sink, delegates to existing
scene commands, delegates display commands to `RuntimeDisplayControls`,
delegates audio commands to `RuntimeAudioControls`, delegates AutoChanger
commands to `RuntimeAutoChangeControls`, delegates legacy EffectControl state
mutations to `RuntimeEffectControls`, delegates ini persistence to
`RuntimePersistence`, delegates close requests to the application-owned
`RuntimeShutdown`, handles palette metadata commands, and reports a
`RuntimeChangeSet`.

The default runtime control implementations still call the current global
display/audio/auto-change/EffectControl objects. Generic `Option*` commands
from the legacy interface are routed only through display, audio, and
owned target adapters; `RuntimeEffectControls` handles EffectControl
selection, effect-choice use flags, and individual EffectControl locks. These
adapters are deliberately thin layers so future subsystem deglobalisation can
replace their internals without teaching `RuntimeChangeMediator` about concrete
subsystem ownership.

## Configuration Flow

Startup config is acquired in `buildStartupConfig()`. A bootstrap command-line
pass only discovers `--path` and `--ini-file`, then `ConfigurationBuilder`
applies defaults, ini files, supported environment variables, and the full
command line into one immutable `Config` value. `Application` handles
diagnostics/help and passes only config slices to subsystem startup APIs.

Without `--ini-file`, the builder searches:

1. `CTH_LIBDIR/cthugha.ini`
2. `~/.cthugha.auto`
3. `~/.cthugha.ini`
4. `./cthugha.ini`
5. `--path DIR` -> `DIR/cthugha.ini`
6. `~/.cthugha.continue`

With `--ini-file FILE`, only that file is opened. Ini chaining from inside an
ini file is intentionally ignored with a warning.

Entry format is:

```text
cthugha.option: value
cthugha.feature.buffer: value
cthugha.feature.buffer.entry: on/off
```

The ini reader rejects `?` wildcard entries with a warning. Pressing `a` at
runtime writes `~/.cthugha.auto` from the registered current runtime config.
