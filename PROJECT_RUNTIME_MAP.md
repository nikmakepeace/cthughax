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
  audioRuntimeInit(RSIC_MainProcess, 1)
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

There is no current `cthugha-server` startup path in source. Earlier docs that
mention `serv_main.cc`, `SoundServer`, or network clients are describing removed
code or stale build artifacts.

## Frame Loop

`run(int doDisplay)` in `src/initExitDisp.cc` is the shared per-frame scheduler.

```text
CthughaDisplay::nextFrame()
  publishes now/deltaT, updates FPS accounting, and enforces maxFPS

audioFrameTick()
  advances AudioRuntime and publishes the current 1024-sample audio frame/window

AudioVisualBridge::runFrame()
  applies selected sound-processing mode
  analyzes raw audio into AudioAnalysis
  updates AcousticContext
  runs AutoChanger

VisualPipeline::run()
  runs visual-stage modules around the legacy buffer transform

CthughaDisplay::operator()()
  frontend-specific display composition, when doDisplay is true

CDPlayer::operator()()
  updates CD playback state

pause/suspend handling
```

The frame loop is cooperative. There are no threads in the visualizer. The code
still uses processes for legacy file playback and translation-table loading.

### Frontend Event Loops

- X11: `DisplayDeviceX11::mainLoop()` drains Xt events, translates key releases,
  dispatches UI work, resizes to the current X window, calls `run(1)`, then runs
  interface input again.
- SVGAlib: `DisplayDeviceSvga::mainLoop()` is still present in source and calls
  `Interface::current->run()` plus `run(1)` in a loop.
- OpenGL: `DisplayDeviceGL` is still present in source; its idle callback calls
  `run(0)` and the GLUT draw callback later invokes `(*cthughaDisplay)()`.

CMake only builds the X11 path.

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
Settings are read from current options (`sound-device-number`, `sound-method`,
`silent`, and the `--play` file name). Environment detection checks OSS read and
write availability and whether Pulse support was compiled in.

`PcmSourceFactory` currently selects:

- `ASS_LineIn` for `SDN_DSPIn`;
- `ASS_Random` for `SDN_Random`;
- `ASS_WavFile` for `--play *.wav`;
- `ASS_Mp3File` for `--play *.mp3`;
- `ASS_RawFile` for other `--play` names, which currently falls back to legacy
  file handling;
- `ASS_Unknown` for unsupported cases.

### Native File Pipeline

For WAV files, and MP3 files when minimp3 is enabled, `AudioRuntime` uses:

```text
PcmSource -> AudioInput -> AudioBuffer -> AudioOutput
                         -> AudioFrameBuilder -> AudioFrame
```

Important pieces:

- `WavPcmSource` parses and reads WAV data.
- `Minimp3PcmSource` decodes MP3 data through `external/minimp3`.
- `AudioOutput` is selected as Pulse, OSS DSP output, or null output.
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

### Legacy SoundDevice Fallback

If no native audio path is available, `AudioRuntime` installs a legacy
`SoundDevice`:

- `SoundDeviceDSPIn`: OSS `/dev/dsp` input.
- `SoundDeviceRandom`: random-noise debug/input source.
- `SoundDeviceFile`: legacy file/fifo/external-program source.
- `SoundDeviceFork`: fork/shared-memory wrapper for legacy non-silent file
  playback.

`SoundDeviceNet` is not a current source file.

### AudioFrame Facade

Visual and UI code should use the facade in `src/AudioFrame.cc`:

```text
audioFrameData()
audioFrameProcessedData()
audioFrameCurrent()
audioFrameTick()
audioFrameChange()
```

These functions choose the current source in this order:

1. `AudioRuntime` frame from the native file pipeline;
2. `AudioInputProcessor` frame from the native input path;
3. legacy `soundDevice`;
4. silent static buffers.

### Sound Processing

The `sound-processing` option is implemented by `src/AudioProcessor.cc`, not by
the old `SoundProcess.cc` file.

Built-in entries:

- `none`: copy raw audio to processed audio.
- `Filter1`: slope-limit sharp sample jumps.
- `Filter2`: low-pass-ish smoothing.
- `FFT`: custom 1024-sample FFT using left/right channels as real/imaginary
  components.

`AudioVisualBridge::runFrame()` calls `audioProcessing.process()` before
analysis and before visual mutation. Waves normally read
`audioFrameProcessedData()`.

### Analysis and Acoustic Context

`AudioAnalyzer::operator()()` computes frame-local `AudioAnalysis`:

- `amplitude`;
- `amplitudeLeft`;
- `amplitudeRight`;
- `noisy`, based on `min-noise`.

`AcousticContext` then maintains rolling state:

- `intensity()`: smoothed normalized amplitude;
- `fire()`: emitted when a rising attack ends;
- `fireLevel()`: accumulated fire used by `AutoChanger`.

`AutoChanger` uses `audioAnalysis.noisy` for silence handling and
`acousticContext.fireLevel()` for fire-driven option changes.

## Visual Buffer Pipeline

`CthughaBuffer` still owns the legacy per-buffer options and the raw active and
passive indexed buffers. Default dimensions are:

```text
BUFF_WIDTH  = 160
BUFF_HEIGHT = 100
```

Each allocation has three hidden rows above and below the visible buffer. Flame
code reads those rows as boundary data.

### VisualPipeline Order

`VisualDirector::planDefaultPipeline()` currently includes these stages:

```text
FlashlightStage
BorderStage
BufferTransformStage
ImageStage        # currently null
FlameStage        # currently null
TranslateStage    # currently null
WaveStage         # currently null
PaletteStage
```

The actual flame/translate/wave work still happens inside
`BufferTransformStage`, which calls `CthughaBuffer::run()` through
`LegacyBufferTransformModule`.

### Flashlight

`apply_flashlight()` is a palette effect. If the `flashlight` CoreOption is on,
it copies the current palette, brightens low palette entries according to
`acousticContext.fire()`, and installs the temporary palette on the
`CthughaFrameBuffer`.

It does not draw pixels into the indexed buffer.

### Border

`apply_border()` fills the three hidden rows above and below the active buffer.

The selected `border` CoreOption decides whether those rows are:

- zero;
- copied from current audio data;
- filled with current amplitude;
- filled with 255.

### Legacy Buffer Transform

`CthughaBuffer::run()` loops over active buffers and applies:

```text
current = buffers + j
done_translate = 0
flame()
translate()
wave()
swap(activeBuffer, passiveBuffer)
```

The order matters:

- `flame` propagates/decays the previous finished image.
- `translate` remaps pixels using loaded translation tables unless the flame
  folded translation into its own loop and set `done_translate`.
- `wave` draws fresh sound-reactive marks into the active buffer.
- swapping makes the finished frame become `passiveBuffer`, which display code
  reads.

Palette smoothing no longer lives in `CthughaBuffer::run()`.

### Palette Stage

`PaletteStageModule` moves the current palette toward the selected palette. The
global `paletteSmoothingChance` controls whether a palette change smooths or
jumps directly to the new palette.

Command-line options:

- `--palette-smoothing VALUE`, clamped to `0.0..1.0`;
- `--no-palette-smoothing`;
- `--palette-set SET`, which filters palettes by metadata set.

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
loaded assets, display functions, GL functions, or no-op entries.

Initial values come from ini files and command-line arguments. Startup applies
them with:

```text
CoreOption::changeToInitial()
audioProcessing.changeToInitial()
```

## Display Pipeline

The classic 2D X11/SVGA display path is:

```text
displayDevice->preDraw()
displayDevice->setGlobalPalette()
choose direct or temporary indexed buffer
checkZoom()
screen() maps passive_buffer into CthughaDisplay::buffer
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
`src/display.cc` for 2D frontends. It maps one `BUFF_WIDTH x BUFF_HEIGHT`
passive buffer into an image that is commonly mirrored to
`2 * BUFF_WIDTH x 2 * BUFF_HEIGHT`.

For OpenGL, retained source in `src/CthughaDisplayGL.cc` uses a different
sequence:

```text
background() before screen
set projection/modelview
light()
screen()
fly()
background() after screen
```

GL display functions in `src/GL_display.cc` upload passive buffers as paletted
textures and draw textured geometry.

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
