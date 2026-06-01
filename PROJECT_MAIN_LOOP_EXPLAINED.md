# Main Loop Explained

This is a guided walk through the current CthughaNix graphical main loop in
`src/`. It describes the active source tree, not removed server-mode paths or
stale build artifacts.

The short version:

```text
frontend event loop
  -> run()
      -> publish frame time
      -> advance audio runtime and current AudioFrame
      -> process/analyze audio and maybe auto-change options
      -> run visual pipeline stages over the indexed visual buffers
      -> draw current passive buffer to the frontend
      -> update CD state
      -> handle deferred suspend
```

Keep these files open:

- `src/initExitDisp.cc`: startup and `run()`.
- `src/Settings.*`: snapshots current audio options.
- `src/AudioRuntime.*`: audio source/output lifecycle.
- `src/RuntimeFactory.*`, `src/PcmSourceFactory.*`: audio strategy selection.
- `src/AudioFrame.*`: facade for the current 1024-sample visual audio frame.
- `src/AudioProcessor.*`: `sound-processing` modes.
- `src/AudioAnalyzer.*`: frame analysis and rolling acoustic state.
- `src/AudioVisualBridge.*`: processing, analysis, and autochanger bridge.
- `src/AutoChanger.*`: automatic effect changes.
- `src/VideoPipeline.*`, `src/VideoPipelineSequence.*`,
  `src/VideoPipelineFactory.*`, `src/PipelineStageModules.*`,
  `src/VideoDirector.*`: visual-stage executor, ordering, composition,
  concrete modules, and policy.
- `src/CthughaBuffer.*`: classic visual buffer dimensions and raw indexed
  active/passive buffers.
- `src/ColorPalette.*`, `src/FramePalette.*`, `src/PaletteTransition.*`:
  palette data, frame palette output, dirty tracking, and palette smoothing.
- `src/flames.cc`, `src/translate.cc`, `src/waves.cc`: classic effect entry
  objects used by the visual stages.
- `src/display.cc`: 2D display mapping effects.
- `src/CthughaDisplay*.cc`: frontend display composition.
- `src/DisplayDevice*.cc`: X11 event loop and screen IO.
- `src/Interface.*`, `src/keymap.*`: key handling and on-screen UI.

## 1. Startup: `main()`

Graphical frontends enter at `src/initExitDisp.cc::main()`.

Startup does this:

```text
srand(time(0))
drop elevated uid
get_pre_params()
cth_init()
get_params()
title()
init_imath()
init_sound()
new CDPlayer
CthughaBuffer::initAll()
init_border()
init_flashlight()
newDisplayDevice()
newCthughaDisplay()
CoreOption::changeToInitial()
audioProcessing.changeToInitial()
Interface::set("main")
Keymap::init()
initAudioVisualBridge()
displayDevice->mainLoop()
```

Read this as "build long-lived singletons, then hand control to the selected
frontend." The code is C++, but ownership is still mostly global:
`displayDevice`, `cthughaDisplay`, `cdPlayer`, `autoChanger`, `audioMetrics`,
and `acousticContext`.

## 2. The Frontend Loop Calls `run()`

The X11 frontend owns the platform event loop and converges on `run()`.

- X11: `DisplayDeviceX11::mainLoop()` handles pending Xt/X events, translates
  key releases, runs the interface, resizes to the X window, then calls
  `run(1)`.

The `doDisplay` argument decides whether `run()` itself calls the display step.
The current X11 frontend passes `1`.

## 3. The Shared Scheduler: `run(int doDisplay)`

The core scheduler is short and worth reading directly:

```text
cthughaDisplay->nextFrame()
audioFrameTick()
runAudioVisualBridge()
runVideoPipeline()
if (doDisplay)
    (*cthughaDisplay)()
(*cdPlayer)()
pause/suspend handling
```

Everything else explains one of these calls.

## 4. Concept: AudioFrame

`AudioFrame` is the current facade between audio backends and visual code.

It contains:

```cpp
long long centerSample;
int samples;
char2 raw[1024];
char2 processedWaveData[1024];
```

Use these functions instead of coupling visual code to a concrete input path:

```cpp
audioFrameRawData();
audioFrameProcessedWaveData();
audioFrameCurrent();
```

They hide whether the current data came from file playback, live/random input,
or silent fallback buffers.

## 5. Concept: CoreOption

`CoreOption` is the runtime registry for visual choices. It is more than a
scalar setting: it owns entries, tracks the current entry, supports locks,
supports random changes, stores history/hotkeys, and can be addressed by
config/keymap/UI code.

Examples:

- `flame`
- `wave`
- `palette`
- `translate`
- `display`
- `border`
- `flashlight`

The visual pipeline uses explicit internal image stages:
`ImageStageModule`, `FlameStageModule`, `TranslateStageModule`, and
`WaveStageModule` render the current domain objects through stage-specific APIs.

Flame selection lives in global `FlameOption` and `GeneralFlameOption` objects
from `src/flames.cc`; `VideoDirector` injects their current values into
`FlameStageModule`.

Wave selection lives in the global wave option state exposed by `src/waves.cc`
for UI/keymap/config code, while `src/Wave.cc` defines the standalone `Wave`
catalog. `VideoDirector` injects the selected `Wave`, wave scale, table, and
object into `WaveStageModule`.

`sound-processing` is adjacent and implemented by `AudioProcessingOption` in
`src/AudioProcessor.cc`. It is selected by command-line/ini option and by the
`m/M` key actions; it is not part of AutoChanger's CoreOption rotation.

## 6. Concept: CthughaBuffer

`CthughaBuffer` is the classic off-screen indexed-color image that flames and
waves modify. It does not own the output palette; the current palette for a
frame lives in `FramePalette`.

Key fields in `src/CthughaBuffer.h`:

```cpp
unsigned char* activeBuffer;
unsigned char* passiveBuffer;
```

During the visual pipeline's indexed-buffer stages:

- `activeBuffer` is the buffer being written for the next finished image.
- `passiveBuffer` is the previous/current finished image.
- `FrameCommitModule` swaps the pointers after flame, translate, and wave
  stages have run.
- Display code reads the buffer's passive pixels, so after the swap it sees
  the newly completed frame.

Default dimensions are in `src/CthughaBuffer.cc`:

```cpp
int BUFF_WIDTH = 160;
int BUFF_HEIGHT = 100;
```

Each buffer allocation has three hidden rows above and below the visible buffer.
The border stage fills those rows before flame code reads them.

## 7. Step 1: Start The Frame

`cthughaDisplay->nextFrame()` is implemented in `src/CthughaDisplay.cc`.

It:

- samples the current time into global `now`;
- computes global `deltaT`;
- updates FPS accounting;
- enforces `maxFPS` by sleeping if needed.

This gives later modules a consistent frame timestamp.

## 8. Step 2: Advance Audio

`audioFrameTick()` calls `audioRuntimeTick()`.

There are two broad cases.

File playback:

```text
AudioInput reads WavPcmSource/Minimp3PcmSource
AudioBuffer queues decoded PCM
AudioOutput writes Pulse, OSS, or null output
AudioFrameBuilder builds the visual frame around the audible sample
```

Live/random input:

```text
PcmSource -> AudioInput -> AudioInputProcessor
```

The result is always exposed as a 1024-sample signed 8-bit stereo visual frame
through `audioFrameRawData()`.

## 9. Step 3: AudioVisualBridge

`runAudioVisualBridge()` ensures an `AudioVisualBridge` exists, then calls
`AudioVisualBridge::runFrame()`.

That method does:

```text
audioProcessing.process()
audioAnalyzer()
autoChanger()
```

It also contains a future pipeline-refresh flag. The current code can ask
`VideoPipelineFactory` to refresh the existing pipeline, although the bridge
does not yet set that flag itself.

## 10. Step 3a: Sound Processing

`audioProcessing.process()` runs the selected entry from `src/AudioProcessor.cc`.

Built-in modes:

- `none`: copy raw frame data to processed wave data.
- `Filter1`: slope-limit sharp sample jumps.
- `Filter2`: low-pass-ish smoothing.
- `FFT`: transform the 1024-sample stereo window and write FFT bins into the
  processed wave data.

Waves normally use `audioFrameProcessedWaveData()`.

## 11. Step 3b: Analyze Audio

`audioAnalyzer()` calls `AudioAnalyzer::operator()()` in `src/AudioAnalyzer.cc`.

It computes `audioMetrics`:

- `amplitudeLeft` and `amplitudeRight`: RMS-like amplitude for each channel.
- `amplitude`: average amplitude.
- `noisy`: whether either side is above `min-noise`.

Then `acousticContext.update(audioMetrics)` maintains rolling state:

- `intensity()`: smoothed normalized amplitude.
- `fire()`: attack event emitted when rising amplitude begins to fall.
- `cumulativeFireLevel()`: accumulated fire used by automatic changing.

"Fire" here is not the visual flame. It is the analysis term for a detected
attack/beat.

## 12. Step 3c: AutoChanger

`(*autoChanger)()` calls `AutoChanger::operator()()` in `src/AutoChanger.cc`.

It decides whether to change visual options automatically:

- If sound has been quiet long enough, it may show a silence message.
- If silence ends after a quiet gap, it can trigger a change.
- If `acousticContext.cumulativeFireLevel()` exceeds `cumulative-fire-level`, it can trigger a
  change.
- If enough time has passed, it can trigger a change.

The actual change is:

```cpp
CoreOption::changeOne()
// or
CoreOption::changeAll()
```

So the autochanger does not directly know about specific flame or wave
functions. It asks the CoreOption system to move current selections.

## 13. Step 4: VideoPipeline

`runVideoPipeline()` initializes the default visual pipeline if needed, builds
a `VideoFrameContext`, lets `VideoDirector` update the current stage objects,
and calls:

```cpp
videoPipeline->run(buffer, context);
```

The context contains:

- current `AudioFrame`, when the native file pipeline has one;
- `AudioMetrics`;
- `AcousticContext`;
- `now`;
- `deltaT`.

The default pipeline currently has these modules:

```text
ImageStageModule
BorderVideoModule
FlameStageModule
TranslateStageModule
WaveStageModule
FrameCommitModule
PaletteStageModule
FlashlightVideoModule
```

Image, flame, translate, and wave are explicit stages. `ImageStageModule`
overlays the selected `IndexedImage` when `VideoDirector` arms the one-shot
image stage. PCX and indexed PNG files are decoded into that shared image
domain object before the frame loop. Before each frame, `VideoDirector`
updates the modules with the selected image, flame, general-flame value,
translation table, wave, border mode, palette target, and flashlight mode. The
pipeline then wraps the current buffer, frame context, and display palette in a
`VideoFrame` and passes that frame through each enabled module.

Important limitation: this is not the final inversion-of-control shape yet.
`VideoDirector` injects selected values into stage modules. Display code still
reads `CthughaBuffer::current` as a compatibility pointer when mapping the
finished passive buffer to the frontend.

## 14. Step 4a: Flashlight

`apply_flashlight()` in `src/Flashlight.cc` is a palette effect.

If `flashlight` is enabled, it:

1. copies the current frame palette;
2. brightens low palette entries according to `acousticContext.fire()`;
3. writes that temporary palette back to `FramePalette`.

It does not draw pixels.

## 15. Step 4b: Border

`apply_border()` in `src/Border.cc` fills the three hidden rows above and below
the active buffer.

The selected `border` CoreOption decides whether those rows are:

- zero;
- copied from audio frame data;
- filled with current amplitude;
- filled with 255.

Flames read neighboring pixels, so these rows are boundary conditions for
diffusion.

## 16. Step 4c: Indexed Buffer Mutation Stages

The indexed-buffer mutation path is implemented as pipeline modules. The
logical order for a frame is:

```text
ImageStageModule
  overlay the selected IndexedImage when VideoDirector has armed ImageStage once

FlameStageModule
  execute bound Flame objects

TranslateStageModule
  execute the bound Translate object

WaveStageModule
  execute the selected Wave with the current scale, table, and object

FrameCommitModule
  emit limited debug summaries
  swap(activeBuffer, passiveBuffer)
```

This order matters. `VideoPipeline::run()` passes one explicit `VideoFrame`
through each enabled module in sequence.

### What Is A Flame?

A flame is a feedback/decay function over the previous image.

It does not usually draw the audio waveform. Instead, it propagates pixels that
already exist: up, down, left, right, watery, faded, generalized, and so on. It
gives the visualizer memory.

In source:

- domain flames are registered in `src/Flame.cc::flameCatalog`;
- `src/flames.cc::_flames` adapts those flames into the current `CoreOption`
  interface through the global `FlameOption`;
- `FlameStageModule` executes the bound `Flame` objects selected by
  `VideoDirector`;
- `init_flames()` precomputes lookup tables such as `divsub`.

Mentally:

```text
previous finished image -> neighboring-pixel math -> next decayed image
```

### What Is Translate?

A translation table is a coordinate remap. For each destination pixel, it says
which source pixel to read:

```text
dst_pixel[i] = src_pixel[translation_table[i]]
```

Loading is in `src/translate.cc::init_translate()`. Built-in
`TranslateGenerator` catalog entries generate tables in-process. By the time the
visualizer runs, selected translations are ready tables. `VideoDirector` passes
the selected table into the translate stage, whose module owns the runtime
`Translate` executor.
Flame and translation are separate stages; translate owns coordinate remapping.

### What Is A Wave?

A wave is the fresh drawing seeded by current sound.

Wave functions read `audioFrameProcessedWaveData()`, `audioMetrics`,
`acousticContext`, and their injected `WaveRuntime`, then draw points, vertical
lines, horizontal lines, spikes, Lissajous shapes, lightning, objects, spirals,
and other geometry into the buffer's active pixels. `WaveStageModule` calls the
selected `Wave::execute(buffer, context, runtime)`.

Those marks become fuel for later frames' flames.

## 17. Step 4d: Palette Stage

`PaletteOption` is the global CoreOption adapter for loaded palettes, and each
`PaletteEntry` wraps a `ColorPalette`. `VideoDirector` binds the selected
entry into `PaletteStageModule`, and the stage delegates the transition math to
`PaletteTransition`. That object stores the target palette, remaining frame
budget, and selected interpolation strategy, then writes the current output
palette into the stage's bound `FramePalette`.
Its transition state is separate from `FramePalette`, so the flashlight overlay
can brighten the displayed palette without feeding back into smoothing.

The display boundary still uses the legacy raw palette shape:

```cpp
typedef unsigned char Palette[256][3];
```

Domain code uses `ColorPalette` around that storage.

The visual buffer stores only 8-bit color indexes. The palette decides what
those indexes mean on screen. `FramePalette::paletteDirty()` tells the display
device whether that meaning changed. Palette smoothing is why color changes can
drift rather than snap.

When a palette change smooths, `VideoDirector` chooses one of three named
strategies at random: RGB linear, RGB squared, or HSL interpolation.

`--palette-smoothing` controls the chance that a palette change smooths instead
of jumping directly.

## 18. Step 5: Draw To The Frontend

If `doDisplay` is true, `run()` calls:

```cpp
(*cthughaDisplay)()
```

For X11, read `src/CthughaDisplayX11.cc::CthughaDisplayX11::operator()()`.

The classic 2D path does this:

```text
displayDevice->setGlobalPalette()
displayDevice->preDraw()
choose output buffer
checkZoom()
while(screen()) draw passive buffer into display buffer
maybe mirror horizontally
expand indexed palette to target pixel format
maybe mirror vertically
clear border
zoom2Screen()
display interface text and errors
displayDevice->postDraw()
```

`run()` measures display latency around this call and feeds it back into
`cthughaDisplay->observeVisualLatency()`. The native file audio pipeline uses
that estimate when choosing the sample window for future frames.

## 19. Concept: What Is `screen()`?

`screen()` is another `CoreOption`, defined in `src/display.cc`:

```cpp
CoreOption screen(-1, "display", screenEntries);
```

A "screen" entry is not the OS window. It is a mapping from the Cthugha buffer
into the larger display buffer.

Examples from `src/display.cc`:

- `screen_up`: copy rows in normal order.
- `screen_down`: flip vertically.
- `screen_2hor`: horizontal split/mirror.
- `screen_4hor`: kaleidoscope.
- `screen_hfield`: heightfield.
- `screen_roll`, `screen_bent`, `screen_plate`: more 3D-ish mappings.

These functions read the visual buffer's passive pixels, which are the
completed Cthugha image after the buffer swap.

## 20. Concept: CthughaDisplay vs DisplayDevice

`CthughaDisplay`

- owns frame timing;
- knows Cthugha buffer geometry;
- runs `screen()`;
- mirrors and zooms;
- expands palette indexes to frontend pixels;
- draws UI text through the device;
- tracks observed visual latency.

`DisplayDevice`

- owns the platform;
- X11 creates windows/images, handles X events, copies to the X server, and can
  use MIT-SHM.

If you are tracing pixels to screen, the route is:

```text
CthughaBuffer passive pixels
  -> src/display.cc screen function
  -> CthughaDisplay buffer/expandedBuffer
  -> DisplayDevice preDraw/postDraw
  -> X11 screen
```

## 21. Step 6: CDPlayer

`(*cdPlayer)()` updates CD-ROM/player state after drawing. The CD player is
separate from the current audio frame; it controls playback devices when CD
support is compiled in.

See `src/CDPlayer.*`.

## 22. Step 7: Suspend Handling

At the end of `run()`, the code checks `cthugha_pause`.

The signal handlers in `src/initExitDisp.cc` set this flag for
`SIGTSTP`/`SIGCONT` behavior. The actual suspend happens only between frame
stages, so the process is not stopped in the middle of a drawing operation.

## 23. Where Keyboard Input Fits

Keyboard input is not inside `run()` directly. It is handled by the frontend
loop and the `Interface` system.

For X11:

```text
DisplayDeviceX11::mainLoop()
  -> collect Xt events
  -> keys_x11(...)
  -> Interface::current->run()
  -> resize display
  -> run(1)
  -> Interface::current->run()
```

`Interface::run()` calls `getkey()` until no key remains. It dispatches keys
through `Keymap::action()`.

The keymap system can:

- change CoreOptions;
- change `sound-processing`, `border`, and `flashlight`;
- enter option/help/CD/screens;
- lock options;
- save/restore hotkeys;
- request quit;
- change sound/display options;
- print screenshots.

Look at:

- `src/default.keymap`
- `src/keymap.cc`
- `src/Interface.cc`
- `src/InterfaceList.cc`
- frontend key translator `src/xwin_keys.cc`.

## 24. A Concrete Source Walk

If you want to step through one frame in your editor:

1. Open `src/initExitDisp.cc`.
2. Read `main()` until `displayDevice->mainLoop()`.
3. Jump to `src/DisplayDeviceX11.cc::mainLoop()`.
4. Return to `src/initExitDisp.cc::run()`.
5. Step into `src/CthughaDisplay.cc::nextFrame()`.
6. Step into `src/AudioFrame.cc::audioFrameTick()`.
7. Step into `src/AudioRuntime.cc::audioRuntimeTick()`.
8. For native file playback, follow `audioRuntimePumpPipeline()` and
   `audioRuntimeBuildFrame()`.
9. For live/random input, follow `AudioInputProcessor::operator()()`.
10. Step into `src/AudioVisualBridge.cc::AudioVisualBridge::runFrame()`.
11. Step into `src/AudioProcessor.cc` for the selected audio processing mode.
12. Step into `src/AudioAnalyzer.cc::AudioAnalyzer::operator()()`.
13. Step into `src/AutoChanger.cc::AutoChanger::operator()()`.
14. Step into `src/VideoPipelineFactory.cc::VideoPipelineFactory::create()` to see
   the current stage ordering.
15. Step into `src/VideoPipeline.cc::VideoPipeline::run()`.
16. Step into `src/Border.cc::apply_border()`.
17. Step through `FlameStageModule`, then jump to the current flame entry in
   `src/flames.cc`.
18. Step through `TranslateStageModule`, then jump to
   `src/translate.cc::Translate::execute()`.
19. Step through `WaveStageModule`, then jump through the current `Wave` in
   `src/Wave.cc` into its drawing function in `src/waves.cc`.
20. Step through `FrameCommitModule` to see the active/passive swap.
21. Return through `PaletteStageModule` in `src/PipelineStageModules.cc`, then into
   `src/PaletteTransition.cc` for palette output.
22. Step into `src/Flashlight.cc::apply_flashlight()` for the palette overlay.
23. Step into `src/CthughaDisplayX11.cc::operator()()`.
24. Jump to the current `screen()` function in `src/display.cc`.
26. Finish in the X11 `DisplayDevice` `postDraw()` path.

## 25. How To Think About One Frame

A single frame is easiest to picture as two layers of behavior.

The sound/control layer:

```text
advance audio source/output
publish a 1024-sample visual frame
process raw audio into processed wave data
compute amplitude/fire/silence
maybe change CoreOptions
```

The image layer:

```text
fill hidden border rows
diffuse previous pixels with a flame
warp pixels with translate
draw fresh audio marks with a wave
swap buffers
smooth palette
brighten palette for flashlight
map buffer to frontend display
```

The essential Cthugha feedback loop is:

```text
wave draws sound into the buffer
flame turns that drawing into motion/trails next frame
translate warps the motion
palette determines the color feel
autochanger uses sound analysis to keep changing the choices
```

That is the project in miniature.
