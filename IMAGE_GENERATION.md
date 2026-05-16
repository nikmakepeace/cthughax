# Image Generation

This is a guided map of how CthughaNix turns sound plus the previous visual frame into pixels on screen.

It starts at the practical level needed for diagnostic visuals. Later sections can be expanded as we inspect each step in `CthughaBuffer::run()` and then the selected `cthughaDisplay->operator()()` implementation.

The very short version:

```text
read/process sound
transform previous image through flame/feedback
optionally translate/warp that image
draw fresh wave marks from sound data
swap buffers
map the finished buffer to the frontend display
send pixels to the platform/window
```

More precisely:

```text
frontend event loop
  -> run()
      -> cthughaDisplay->nextFrame()
      -> (*soundDevice)()
      -> soundAnalyze()
      -> maybe autochange/options/server
      -> CthughaBuffer::run()
          -> soundProcess()
          -> flashlight()
          -> border setup
          -> flame()
          -> translate()
          -> wave()
          -> smoothPalette()
          -> swap active/passive buffers
      -> (*cthughaDisplay)()
          -> screen()
          -> mirror/expand/zoom/composite
          -> displayDevice->postDraw()
```

## Important Files

- `src/initExitDisp.cc`: the shared `run(int doDisplay)` scheduler.
- `src/CthughaBuffer.cc`: the core image-generation path.
- `src/CthughaBuffer.h`: buffer fields and the `active_buffer` / `passive_buffer` macros.
- `src/SoundProcess.cc`: converts raw sound into `soundDevice->dataProc`.
- `src/flames.cc`: feedback effects that transform the previous image.
- `src/translate.cc`: coordinate remapping effects.
- `src/waves.cc`: fresh drawing seeded by processed sound.
- `src/display.cc`: 2D `screen()` mappings from Cthugha buffer to display buffer.
- `src/CthughaDisplay*.cc`: frontend-specific display composition.
- `src/DisplayDevice*.cc`: platform/window IO.
- `src/GL_display.cc`: OpenGL display mappings and texture upload path.

## The Frame Scheduler

The shared frame scheduler is `run(int doDisplay)` in `src/initExitDisp.cc`.

The relevant image path is:

```cpp
cthughaDisplay->nextFrame();
(*soundDevice)();
soundAnalyze();
(*autoChanger)();
(*soundServer)();
CthughaBuffer::run();

if (doDisplay)
    (*cthughaDisplay)();
```

`nextFrame()` updates the shared frame time values `now`, `deltaT`, and FPS bookkeeping. Sound is then read and analyzed before any image-generation work runs.

The `doDisplay` argument depends on the frontend:

- X11 calls `run(1)`, so display happens inside `run()`.
- SVGAlib calls `run(1)`, also displaying inside `run()`.
- OpenGL calls `run(0)` from its idle callback, then GLUT later calls `(*cthughaDisplay)()` from the draw callback.

So OpenGL separates "generate the next Cthugha image" from "draw the current OpenGL frame", but it still uses the same generated `passiveBuffer`.

## The Two Buffers

`CthughaBuffer` owns two indexed-color image buffers:

```cpp
unsigned char* activeBuffer;
unsigned char* passiveBuffer;
```

The macros in `src/CthughaBuffer.h` expose them as:

```cpp
#define active_buffer (CthughaBuffer::current->activeBuffer)
#define passive_buffer (CthughaBuffer::current->passiveBuffer)
```

At the boundary of `CthughaBuffer::run()`:

- `passiveBuffer` is the previous completed frame.
- `activeBuffer` is the buffer being written for the next completed frame.
- `wave()` draws into `active_buffer`.
- `screen()` reads from `passive_buffer`.
- At the end of `CthughaBuffer::run()`, the pointers are swapped so the newly generated image becomes the display-readable `passiveBuffer`.

After `CthughaBuffer::run()` completes, `passiveBuffer` contains the newly completed Cthugha image for this frame:

```text
previous completed frame
  -> transformed by flame feedback
  -> optionally translated/warped
  -> drawn over by the current wave effect using current processed sound
  -> made display-readable by the final active/passive swap
```

It does not include:

- `screen()` mapping effects;
- mirroring or kaleidoscope display layout;
- zooming or scaling;
- native X11/SVGA/GL pixels;
- UI text or errors;
- the final frontend copy to the window.

There is one subtlety: some flame and translate functions swap the pointers internally while they work. For diagnostics, the safest mental model is to trust the stage boundaries rather than the names inside individual effects.

<a id="cthugha-buffer-run"></a>

## `CthughaBuffer::run()`

The core buffer path is in `src/CthughaBuffer.cc`:

| Order | Code | Section |
| --- | --- | --- |
| 1 | [`current->soundProcess()`](#soundprocess) | [soundProcess](#soundprocess) |
| 2 | [`current->flashlight()`](#flashlight) | [flashlight](#flashlight) |
| 3 | [`switch (int(current->border))`](#border-setup) | [Border Setup](#border-setup) |
| 4 | [`current->flame()`](#flame) | [flame](#flame) |
| 5 | [`current->translate()`](#translate) | [translate](#translate) |
| 6 | [`current->wave()`](#wave) | [wave](#wave) |
| 7 | [`current->smoothPalette()`](#smoothpalette) | [smoothPalette](#smoothpalette) |
| 8 | [`activeBuffer` / `passiveBuffer` swap](#final-swap) | [Final Swap](#final-swap) |

This is the main image-generation pipeline.

<a id="soundprocess"></a>

### `soundProcess()`

This prepares the sound data that waves normally read.

The raw sound samples live in:

```cpp
soundDevice->data
```

The processed copy lives in:

```cpp
soundDevice->dataProc
```

The selected `sound-process` option can copy raw samples, filter them, or run an FFT. Most wave functions read `dataProc`, not `data`.

<a id="flashlight"></a>

### `flashlight()`

This is a palette/intensity effect, not a geometry drawing step. It adjusts the current palette based on sound analysis so low palette entries can brighten with the audio.

<a id="border-setup"></a>

### Border Setup

`CthughaBuffer::run()` writes three hidden rows above and below `active_buffer`. Flame functions can read these rows as boundary input.

The border can be:

- zeroed;
- copied from raw sound data;
- filled with the current analyzed amplitude;
- filled with `255`.

<a id="flame"></a>

### `flame()`

Flame is the feedback step. It usually reads the previous image and writes a decayed, shifted, averaged, or otherwise transformed version into the next image buffer.

This is why the visualizer has memory. Without this step, the wave drawing would mostly be immediate marks instead of trails, smears, flows, and motion.

Some flame functions use the `PTR` macro in `src/flames.cc`, which swaps `active_buffer` and `passive_buffer` internally before processing. Other flame functions read from `passive_buffer` and write into `active_buffer` without that macro.

<a id="translate"></a>

### `translate()`

Translation is an optional coordinate remap. It can warp, swirl, rotate, fold, or otherwise remap the image after the flame step.

If a flame variant has already incorporated the translation table, it sets:

```cpp
CthughaBuffer::current->done_translate = 1;
```

Then `translate()` returns without doing duplicate work.

<a id="wave"></a>

### `wave()`

Wave is the fresh drawing step. It reads processed sound and writes new marks into `active_buffer`.

Common helpers in `src/waves.cc` include:

- `prepareSoundData()`: resamples `soundDevice->dataProc` to the number of visual samples needed.
- `putat()` / `putat_cut()`: draw small point/cross marks.
- `do_vwave()`: draw vertical wave segments.
- `do_hwave()`: draw horizontal wave segments.
- `draw_line()`: draw line geometry.
- `tcolor(x)`: map a sound-derived value through the selected sound color table.

Conceptually:

```text
processed sound samples -> positions/colors -> new pixels in active_buffer
```

Those pixels then become fuel for the next frame's feedback step.

#### Diagnostic Grid Wave

There are two diagnostic wave entries in `src/waves.cc`:

- `Grid`: draws a fixed indexed-color grid with borders, center axes, diagonals, and corner markers, leaving interstitial pixels untouched.
- `None`: draws nothing.

Both entries are registered with `inUse = 0`. Normal wave cycling and random changes skip them, but they can still be selected explicitly by name. Selecting one by name turns that entry on through the normal `CoreOption::change()` path.

This is useful when testing the pixel permutation path.

Because `wave()` runs after `flame()` and `translate()`, `Grid` does not pass through flame/translate in the same frame where it is drawn. Instead, it becomes the completed `passiveBuffer` after the final swap, then becomes input to `flame()` and `translate()` on the next frame.

The useful workflow is:

```text
select wave Grid
  -> seed passiveBuffer with a known regular source image

then select wave None
  -> stop adding fresh wave pixels
  -> watch flame/translate/screen transform the seeded grid over later frames
```

If `Grid` stays selected, it redraws only the grid pixels every frame. The spaces between grid lines still come from the flame/translate result, so this behaves more like a diagnostic overlay than a full replacement image.

<a id="smoothpalette"></a>

### `smoothPalette()`

The buffers store 8-bit palette indexes, not final RGB pixels. `smoothPalette()` gradually moves `currentPalette` toward the selected palette, so palette changes can drift instead of snapping.

<a id="final-swap"></a>

### Final Swap

At the end of `CthughaBuffer::run()`:

```cpp
unsigned char* t = current->activeBuffer;
current->activeBuffer = current->passiveBuffer;
current->passiveBuffer = t;
```

After this swap, `passiveBuffer` is the newly completed image. That is the image the display subsystem reads. It is not RGB screen pixels. It is a `BUFF_WIDTH * BUFF_HEIGHT` array of 8-bit palette indexes. Each byte is a color index from `0` to `255`; the actual RGB meaning comes later from `currentPalette` during display or palette expansion.

For diagnostics, this is the cleanest point to capture "what Cthugha generated this frame." In the 2D path, `screen()` reads this buffer and maps it into `cthughaDisplay->buffer`. In the OpenGL path, GL display code can upload or sample it as a paletted texture or height field.

<a id="display-operator"></a>

## Display: `cthughaDisplay->operator()()`

After `CthughaBuffer::run()` has produced the completed indexed image, the frontend display object maps it to the actual output target.

This is a second visual stage. `CthughaBuffer::run()` generates the raw Cthugha image, but the selected `screen()` function can still flip, mirror, split, bend, roll, or otherwise transform that image for presentation.

```text
sound + previous frame
  -> CthughaBuffer::run()
  -> passiveBuffer: raw generated indexed image

passiveBuffer
  -> screen_* display mapping
  -> cthughaDisplay->buffer / expandedBuffer
  -> mirror/palette expansion/zoom
  -> frontend/window
```

There are three main frontend variants:

- `src/CthughaDisplayX11.cc`
- `src/CthughaDisplaySVGA.cc`
- `src/CthughaDisplayGL.cc`

The 2D X11/SVGA path is roughly:

| Order | Step | Notes |
| --- | --- | --- |
| 1 | `displayDevice->preDraw()` | Prepare the platform/device buffer. |
| 2 | choose display/scratch buffer | Direct mode draws to device memory; mapped modes use a scratch buffer. |
| 3 | `checkZoom()` | Compute output size and border behavior. |
| 4 | [`screen()`](#screen) | Map `passive_buffer` into the display buffer. |
| 5 | mirror if needed | Complete the 2x2 logical image. |
| 6 | expand palette if needed | Convert indexed pixels to native frontend pixels. |
| 7 | clear border | Clear areas outside the image rectangle. |
| 8 | zoom/copy to device buffer | Copy or scale the display buffer to the output area. |
| 9 | draw text/errors | Overlay interface text and errors. |
| 10 | `displayDevice->postDraw()` | Push the final pixels to the frontend. |

The key call is:

```cpp
while (screen())
    ;
```

<a id="screen"></a>

The selected [`screen()`](#screen) function reads from `passive_buffer` and writes into `cthughaDisplay->buffer`. It is not the OS screen. It is a display mapping effect that runs after the source image has already been generated.

Examples in `src/display.cc` include:

- `screen_up`: plain copy.
- `screen_down`: vertical flip.
- `screen_2hor` / `screen_r2hor`: split and mirror arrangements.
- `screen_4hor` / `screen_4kal`: kaleidoscope-style mappings.
- `screen_hfield`: heightfield-style presentation.
- `screen_roll`: roll around an axis.
- `screen_bent`: bent plane presentation.
- `screen_plate`: rotating plate presentation.

OpenGL uses a different display path. Its `screen()` entries live in `src/GL_display.cc`; they upload or sample `CthughaBuffer::buffers[P].passiveBuffer` as paletted textures or height data, then render GL objects/planes.

This distinction matters for diagnostics. A frame can be correct in `passiveBuffer` but look very different on screen because the selected `display` / `screen()` option transformed it.

## Diagnostic Hook Points

Useful early hook points:

- After `soundProcess()`: inspect raw sound versus processed `dataProc`.
- After border setup: inspect hidden boundary rows.
- After `flame()`: inspect feedback before explicit translation.
- After `translate()`: inspect warped feedback before fresh wave marks.
- After `wave()`: inspect the next image before palette smoothing and swap.
- Immediately after the final swap: inspect exactly what display will read.
- Inside `screen()`: inspect mapping from `passive_buffer` to display buffer.
- After palette expansion: inspect final native pixels before zoom/copy.
- Before `postDraw()`: inspect final frontend buffer.

For most image-generation diagnostics, start in `CthughaBuffer::run()`. For "why does it look different on screen than in the Cthugha buffer?", move into `cthughaDisplay->operator()()` and the selected `screen()` function.

The two most useful comparison captures are:

- after `CthughaBuffer::run()`: the raw generated frame in `passiveBuffer`;
- after `screen()`: the mapped presentation frame in `cthughaDisplay->buffer`.
