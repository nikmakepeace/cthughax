# Main Loop Explained

This is a guided walk through the current CthughaNix graphical main loop in
`src/`. It describes the refactored tree, not the older `SoundAnalyze` /
`SoundProcess` / sound-server architecture.

The short version:

```text
frontend event loop
  -> run()
      -> publish frame time
      -> advance audio runtime and current AudioFrame
      -> process/analyze audio and maybe auto-change options
      -> run visual pipeline around the legacy buffer transform
      -> draw current passive buffer to the frontend
      -> update CD state
      -> handle deferred suspend
```

Keep these files open:

- `src/initExitDisp.cc`: startup and `run()`.
- `src/AudioRuntime.*`: audio source/output lifecycle.
- `src/RuntimeFactory.*`, `src/PcmSourceFactory.*`: audio strategy selection.
- `src/AudioFrame.*`: facade for the current 1024-sample visual audio frame.
- `src/AudioProcessor.*`: `sound-processing` modes.
- `src/AudioAnalyzer.*`: frame analysis and rolling acoustic state.
- `src/AudioVisualBridge.*`: processing, analysis, and autochanger bridge.
- `src/AutoChanger.*`: automatic effect changes.
- `src/VisualPipeline.*`, `src/VisualDirector.*`: visual-stage scaffold.
- `src/CthughaFrameBuffer.*`: indexed buffer/palette adapter.
- `src/CthughaBuffer.*`: legacy visual buffer and flame/translate/wave driver.
- `src/flames.cc`, `src/translate.cc`, `src/waves.cc`: classic effects.
- `src/display.cc`: 2D display mapping effects.
- `src/CthughaDisplay*.cc`: frontend display composition.
- `src/DisplayDevice*.cc`: X11, SVGAlib, or OpenGL event loops and screen IO.
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
audioRuntimeInit(RSIC_MainProcess, 1)
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
`displayDevice`, `cthughaDisplay`, `cdPlayer`, `autoChanger`, `audioAnalysis`,
`acousticContext`, and sometimes the legacy `soundDevice`.

## 2. The Frontend Loop Calls `run()`

Each frontend owns the platform event loop, but they all converge on `run()`.

- X11: `DisplayDeviceX11::mainLoop()` handles pending Xt/X events, translates
  key releases, runs the interface, resizes to the X window, then calls
  `run(1)`.
- SVGAlib: `DisplayDeviceSvga::mainLoop()` remains in source and calls
  `Interface::current->run()` and `run(1)` in a loop.
- OpenGL: retained GLUT code calls `run(0)` from idle and draws later from the
  display callback.

The `doDisplay` argument decides whether `run()` itself calls the display step.
X11 and SVGAlib pass `1`; OpenGL passes `0` because GLUT separates idle updates
from drawing callbacks.

CMake currently builds only the X11 frontend.

## 3. The Shared Scheduler: `run(int doDisplay)`

The core scheduler is short and worth reading directly:

```text
cthughaDisplay->nextFrame()
audioFrameTick()
runAudioVisualBridge()
runVisualPipeline()
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
char2 data[1024];
char2 processed[1024];
```

Use these functions instead of reading `soundDevice` directly:

```cpp
audioFrameData();
audioFrameProcessedData();
audioFrameCurrent();
```

They hide whether the current data came from the native file pipeline, a native
input processor, a legacy `SoundDevice`, or silent fallback buffers.

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

The pattern is:

```cpp
current->flame();
current->wave();
screen();
```

Those calls invoke the currently selected `CoreOptionEntry::operator()()`, not a
hard-coded function.

`sound-processing` is adjacent but now implemented by `AudioProcessingOption`
in `src/AudioProcessor.cc`.

## 6. Concept: CthughaBuffer

`CthughaBuffer` is the classic off-screen indexed-color image that flames and
waves modify.

Key fields in `src/CthughaBuffer.h`:

```cpp
unsigned char* activeBuffer;
unsigned char* passiveBuffer;
```

During the legacy transform:

- `activeBuffer` is the buffer being written for the next finished image.
- `passiveBuffer` is the previous/current finished image.
- At the end of the transform, the pointers are swapped.
- Display code reads from `passive_buffer`, so after the swap it sees the newly
  completed frame.

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

There are three broad cases.

Native file playback:

```text
AudioInput reads WavPcmSource/Minimp3PcmSource
AudioBuffer queues decoded PCM
AudioOutput writes Pulse, OSS, or null output
AudioFrameBuilder builds the visual frame around the audible sample
```

Native live/random input:

```text
PcmSource -> AudioInput -> AudioInputProcessor
```

Legacy fallback:

```text
SoundDevice::operator()()
```

The result is always exposed as a 1024-sample signed 8-bit stereo visual frame
through `audioFrameData()`.

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
`VisualPipelineFactory` to refresh the existing pipeline, although the bridge
does not yet set that flag itself.

## 10. Step 3a: Sound Processing

`audioProcessing.process()` runs the selected entry from `src/AudioProcessor.cc`.

Built-in modes:

- `none`: copy raw frame data to processed frame data.
- `Filter1`: slope-limit sharp sample jumps.
- `Filter2`: low-pass-ish smoothing.
- `FFT`: transform the 1024-sample stereo window and write FFT bins into the
  processed frame.

Waves normally use `audioFrameProcessedData()`.

## 11. Step 3b: Analyze Audio

`audioAnalyzer()` calls `AudioAnalyzer::operator()()` in `src/AudioAnalyzer.cc`.

It computes `audioAnalysis`:

- `amplitudeLeft` and `amplitudeRight`: RMS-like amplitude for each channel.
- `amplitude`: average amplitude.
- `noisy`: whether either side is above `min-noise`.

Then `acousticContext.update(audioAnalysis)` maintains rolling state:

- `intensity()`: smoothed normalized amplitude.
- `fire()`: attack event emitted when rising amplitude begins to fall.
- `fireLevel()`: accumulated fire used by automatic changing.

"Fire" here is not the visual flame. It is the analysis term for a detected
attack/beat.

## 12. Step 3c: AutoChanger

`(*autoChanger)()` calls `AutoChanger::operator()()` in `src/AutoChanger.cc`.

It decides whether to change visual options automatically:

- If sound has been quiet long enough, it may show a silence message.
- If silence ends after a quiet gap, it can trigger a change.
- If `acousticContext.fireLevel()` exceeds `fire-level`, it can trigger a
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

## 13. Step 4: VisualPipeline

`runVisualPipeline()` initializes the default visual pipeline if needed, binds a
`CthughaFrameBuffer` to the current legacy buffer pointers, builds a
`VisualFrameContext`, and calls:

```cpp
visualPipeline->run(visualFrameBuffer, context);
```

The context contains:

- current `AudioFrame`, when the native file pipeline has one;
- `AudioAnalysis`;
- `AcousticContext`;
- `now`;
- `deltaT`.

The default pipeline currently has these modules:

```text
FlashlightVisualModule
BorderVisualModule
LegacyBufferTransformModule
NullVisualStageModule("image")
NullVisualStageModule("flame")
NullVisualStageModule("translate")
NullVisualStageModule("wave")
PaletteStageModule
```

The null modules are placeholders for future decomposition. Most classic visual
mutation is still inside `LegacyBufferTransformModule`.

## 14. Step 4a: Flashlight

`apply_flashlight()` in `src/Flashlight.cc` is a palette effect.

If `flashlight` is enabled, it:

1. copies the current frame palette;
2. brightens low palette entries according to `acousticContext.fire()`;
3. installs that temporary palette on the frame buffer.

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

## 16. Step 4c: Legacy Buffer Transform

`LegacyBufferTransformModule` calls `CthughaBuffer::run()`.

For every active buffer:

```text
current = buffers + j
done_translate = 0
flame()
translate()
wave()
swap(activeBuffer, passiveBuffer)
```

This order matters.

### What Is A Flame?

A flame is a feedback/decay function over the previous image.

It does not usually draw the audio waveform. Instead, it propagates pixels that
already exist: up, down, left, right, watery, faded, generalized, and so on. It
gives the visualizer memory.

In source:

- entries are registered in `src/flames.cc::_flames`;
- each entry is a `FlameEntry`;
- `init_flames()` precomputes lookup tables such as `divsub`.

Mentally:

```text
old finished image -> neighboring-pixel math -> new decayed image
```

### What Is Translate?

A translation table is a coordinate remap. For each destination pixel, it says
which source pixel to read:

```text
dst_pixel[i] = src_pixel[translation_table[i]]
```

Loading is in `src/translate.cc::init_translate()`. `.cmd` files can generate
tables with helper programs, and `.tab` files can be loaded directly. Tables can
be loaded on demand.

Some flame paths fold translation into the flame step and set `done_translate`,
so the separate translate pass skips duplicate work.

### What Is A Wave?

A wave is the fresh drawing seeded by current sound.

Wave functions read `audioFrameProcessedData()`, `audioAnalysis`,
`acousticContext`, `waveScale`, and `table`, then draw points, vertical lines,
horizontal lines, spikes, Lissajous shapes, lightning, objects, spirals, and
other geometry into `active_buffer`.

Those marks become fuel for later frames' flames.

## 17. Step 4d: Palette Stage

`PaletteStageModule` moves the current palette toward the selected palette over
time.

Palettes are 256 entries of RGB:

```cpp
typedef unsigned char Palette[256][3];
```

The visual buffer stores only 8-bit color indexes. The palette decides what
those indexes mean on screen. Palette smoothing is why color changes can drift
rather than snap.

`--palette-smoothing` controls the chance that a palette change smooths instead
of jumping directly.

## 18. Step 5: Draw To The Frontend

If `doDisplay` is true, `run()` calls:

```cpp
(*cthughaDisplay)()
```

For X11, read `src/CthughaDisplayX11.cc::CthughaDisplayX11::operator()()`.
For SVGAlib, read `src/CthughaDisplaySVGA.cc::CthughaDisplaySVGA::operator()()`.
For OpenGL, read `src/CthughaDisplayGL.cc::CthughaDisplayGL::operator()()`.

The classic 2D path does this:

```text
displayDevice->preDraw()
displayDevice->setGlobalPalette()
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

These functions read `passive_buffer`, which is the completed Cthugha image
after the buffer swap.

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
  use MIT-SHM;
- SVGAlib owns console graphics buffers;
- OpenGL owns GLUT callbacks, GL state, and buffer swaps.

If you are tracing pixels to screen, the route is:

```text
CthughaBuffer passive_buffer
  -> src/display.cc screen function
  -> CthughaDisplay buffer/expandedBuffer
  -> DisplayDevice preDraw/postDraw
  -> X11/SVGA/GL screen
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
- frontend key translators such as `src/xwin_keys.cc`, `src/nonx_keys.cc`, and
  `src/GL_keys.cc`.

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
14. Step into `src/VisualDirector.cc::VisualPipelineFactory::create()` to see
   the current stage ordering.
15. Step into `src/VisualPipeline.cc::VisualPipeline::run()`.
16. Step into `src/Flashlight.cc::apply_flashlight()`.
17. Step into `src/Border.cc::apply_border()`.
18. Step into `src/CthughaBuffer.cc::CthughaBuffer::run()`.
19. Jump to the current flame in `src/flames.cc`.
20. Jump to `src/translate.cc::TranslateEntry::operator()()` if translate is
   enabled.
21. Jump to the current wave in `src/waves.cc`.
22. Return through `PaletteStageModule` in `src/VisualDirector.cc`.
23. Step into `src/CthughaDisplayX11.cc::operator()()`.
24. Jump to the current `screen()` function in `src/display.cc`.
25. Finish in the X11 `DisplayDevice` `postDraw()` path.

## 25. How To Think About One Frame

A single frame is easiest to picture as two layers of behavior.

The sound/control layer:

```text
advance audio source/output
publish a 1024-sample visual frame
process raw audio into processed audio
compute amplitude/fire/silence
maybe change CoreOptions
```

The image layer:

```text
brighten palette for flashlight
fill hidden border rows
diffuse old pixels with a flame
warp pixels with translate
draw fresh audio marks with a wave
swap buffers
smooth palette
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
