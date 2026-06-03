# Current To Target Display Architecture Delta

## Purpose

This document compares the current display architecture with the target display
architecture described in `DisplayTargetArchitecture.md`.

The current code has already moved in a useful direction: the video filterchain
can publish an `IndexedFrame`, the display path owns an `IndexedDisplayFrame`,
and the screen catalog has been separated into `Screen.h`/`Screen.cc`.

The remaining problem is that the display path is still coordinated through
global state and platform-specific control flow. It is not yet inverted far
enough for screen effects, presentation composition, scaling, overlay drawing,
and platform presentation to be tested independently.

## Current Construction

`Application::initialize()` currently performs display startup in this order:

```text
cth_init(&argc, argv)
newDisplayDevice(scene, sceneCommands)
newCthughaDisplay()
```

Effects of that construction:

- `newDisplayDevice(...)` constructs `DisplayDeviceX11` and stores it in the
  global `displayDevice`.
- `newCthughaDisplay()` constructs `CthughaDisplayX11` and stores it in the
  global `cthughaDisplay`.
- `screen` is a global `EffectControl` backed by the global screen catalog.
- `zoom`, `draw_size`, `disp_size`, `bypp`, `bytes_per_line`, and `draw_mode`
  are global display state.
- `Interface::current` is global UI state and is invoked by the display path.

The construction shape means `Application` starts the display system, but it
does not truly own a display object graph. It owns shutdown responsibility for
global pointers.

Target delta:

- `Application` should own a `DisplayRuntime` object graph.
- Display factories should return objects rather than mutating globals.
- During transition, globals can remain as compatibility aliases, but ownership
  should move to `Application`.

## Current Frame Flow

At runtime, `Application::runFrame()` does this:

```text
cthughaDisplay->nextFrame()
audioFrameTick()
runAudioVisualBridge()
runVideoFilterchain() -> const IndexedFrame*
cthughaDisplay->present(indexedFrame)
```

`CthughaDisplay::present()` stores the frame pointer in `sourceFrame`, updates
the display device palette pointer, then calls virtual `operator()`.

For X11, `CthughaDisplayX11::operator()` currently does all of this:

```text
displayDevice->setGlobalPalette()
screenEntry = (ScreenEntry*)screen.current()
prepareIndexedDisplayFrame(screenEntry->outputSize(...))
display_base = displayDevice->preDraw()
prepareExpandedBuffer()
choose buffer and bufferWidth from draw_mode
choose expandedBuffer and expandedBufferWidth from bypp
checkZoom()
while (screen()) ;       // calls selected screen renderer
mirror incomplete horizontal region if filled width < output width
expandPalette(filled height)
mirror incomplete vertical region if filled height < output height
clearBorder()
zoom2Screen(display_base + SCREEN_OFFSET, bytes_per_line)
displayDevice->prePrint()
Interface::current->display()
errors.display()
displayDevice->postPrint()
displayDevice->postDraw()
```

Target delta:

- Screen selection and screen rendering should move to a display-agnostic
  `PresentationComposer`.
- X11 should receive an already composed indexed display frame.
- Viewport calculation should be explicit data rather than `SCREEN_OFFSET`
  macros and global `draw_size`.
- Overlay collection should be outside backend presentation.
- X11 should present a `DisplayPresentation` rather than reaching into global
  screen and interface state.

## Current Object Responsibilities

### Application

Current responsibilities:

- Owns high-level lifecycle and main loop.
- Initializes display globals.
- Calls `cthughaDisplay->present(...)`.
- Deletes global display objects during shutdown.

Target gap:

- It should own a real display runtime graph instead of global aliases.

### CthughaDisplay

Current responsibilities:

- Stores the current source `IndexedFrame`.
- Owns an `IndexedDisplayFrame`.
- Owns FPS timing and visual latency.
- Exposes source and destination accessors used by screen functions.
- Provides mirror helpers, border clearing, zoom/scaling copy, frame pacing, and
  status text.

Target gap:

- It is too broad. Its responsibilities should split into frame clock/pacer,
  presentation composer, viewport policy, and display runtime facade.

### CthughaDisplayX11

Current responsibilities:

- Coordinates the entire display frame.
- Selects and runs screen effects.
- Allocates true-color expansion buffers.
- Expands palette.
- Calls X11 display device pre/post draw.
- Triggers overlay drawing.

Target gap:

- It should become either the X11 backend or disappear behind an X11 backend.
- It should not select screen effects or call UI singletons.

### DisplayDevice

Current responsibilities:

- Abstracts platform drawing enough for X11.
- Owns text drawing primitives.
- Holds palette state, text flags, full-copy flags, and global-ish display
  settings.

Target gap:

- It mixes backend state, overlay text rendering, palette dirty logic, and
  presentation copy semantics.
- It should become a backend interface plus separate overlay/text helpers, or an
  adapter while a new `DisplayBackend` interface is introduced.

### DisplayDeviceX11

Current responsibilities:

- Owns X11 window/resources.
- Processes X11 events.
- Allocates XImage/SHM resources.
- Handles palette setup and palette preview UI.
- Handles root-window/fullscreen/window resize behavior.
- Copies image or pixmap to the window.
- Draws text through X11 or panel widgets.

Target gap:

- These are valid backend responsibilities, but they are exposed through global
  state and old display primitives.
- X11-specific optional panel behavior may remain in X11, but screen
  presentation decisions should not.

### ScreenEntry

Current responsibilities:

- Holds renderer function pointer.
- Holds name/description/enabled state through `EffectChoice`.
- Holds filled scale and output scale.
- Executes a no-argument renderer.

Target gap:

- Renderer execution lacks an explicit `ScreenRenderContext`.
- Completion behavior is still inferred by comparing filled size and output
  size, rather than being an explicit rendering instruction.

### Screen renderers in display.cc

Current responsibilities:

- Read current source pixels through helper functions that call global
  `cthughaDisplay`.
- Write into `cthughaDisplay->buffer`.
- Use `cthughaDisplay->bufferWidth`.
- Some renderers change the selected `screen` option when geometry is unsuitable.
- Some renderers use global timing/FPS.

Target gap:

- They should receive all source/destination/timing/select-next state through
  `ScreenRenderContext`.
- They should not know `cthughaDisplay`.

### IndexedFrame

Current responsibilities:

- Borrow source indexed pixels, dimensions, pitch, and palette from the
  filterchain.

Target gap:

- This is close to target.
- Tests should continue protecting pitch handling.

### IndexedDisplayFrame

Current responsibilities:

- Own display-agnostic indexed presentation pixels.
- Own dimensions, pitch, capacity, and frame palette.

Target gap:

- This is close to target.
- It should be owned by `PresentationComposer`, not by a broad display/X11
  coordinator.

## Coupling Delta

| Area | Current Coupling | Target Coupling |
| --- | --- | --- |
| Main loop | `Application` calls global-backed `displayDevice` and `cthughaDisplay` | `Application` calls owned `DisplayRuntime` |
| Display construction | Factories mutate global pointers | Factories return owned objects |
| Screen selection | X11 presentation reads global `screen.current()` | `DisplayRuntime` or `PresentationComposer` receives `ScreenSelection` |
| Screen rendering | Renderers read/write through global `cthughaDisplay` | Renderers use `ScreenRenderContext` |
| Completion | X11 infers mirror from filled/output size | Composer applies explicit completion strategy |
| Palette expansion | X11 display coordinator expands palette before final copy | Backend or `PixelTransfer` converts indexed frame to native pixels |
| Scaling | `zoom2Screen()` uses global `zoom`, `disp_size`, `draw_size`, `bypp` | `ViewportPolicy` returns explicit `DisplayViewport`; backend implements |
| Overlay | X11 presentation calls `Interface::current->display()` and `errors.display()` | Runtime collects overlay commands and backend renders them |
| Events | Application calls global `displayDevice->processEvents()` | Application calls `displayRuntime.processEvents()` |
| Timing | `CthughaDisplay` owns shared `now` and `deltaT` globals | `FrameClock` owns frame timing and publishes/injects it |
| Testing | Meaningful display tests need global setup or X11-adjacent objects | Composer, viewport, transfer, and renderers test without backend |

## What Is Already Close To Target

`IndexedFrame` is already a good source handoff object.

- It carries pixels, width, height, pitch, and palette.
- It is display-agnostic.

`IndexedDisplayFrame` is a good presentation destination object.

- It owns indexed output pixels.
- It carries dimensions and pitch.
- It is separate from native display memory.

The screen catalog is separated from `display.cc`.

- `Screen.h`/`Screen.cc` give screen entries a typed home.
- `display.cc` can become only renderer implementation code.

The application lifecycle is clearer than before.

- `Application::runFrame()` already has a visible handoff from filterchain to
  display.
- This gives us a natural place to introduce `DisplayRuntime`.

## What Is Furthest From Target

Screen renderer context is the biggest gap.

- Every renderer depends on `cthughaDisplay` as an implicit context.
- This blocks no-window tests for screen behavior.

X11 presentation is too central.

- `CthughaDisplayX11::operator()` is currently the display coordinator,
  presentation composer, palette expander, scaler, overlay dispatcher, and
  backend presenter.

Display settings are global and mutable.

- `zoom`, `draw_size`, `disp_size`, `bypp`, `bytes_per_line`, and `draw_mode`
  are read from many places.
- Some are settings, some are backend facts, and some are derived per-frame
  geometry. They need to be separated.

Overlay is embedded in presentation.

- Backend text drawing and UI state are coupled.
- Text palette darkening and full-copy behavior must be preserved while the
  ownership is cleaned up.

## Current Performance Shape

Current strengths:

- Indexed source data stays compact.
- `IndexedDisplayFrame` reuses capacity.
- X11 true-color palette expansion uses lookup tables.
- X11 SHM paths can avoid slow copies to the server.

Current weaknesses:

- Scaling and palette expansion are mixed with backend presentation, making it
  hard to choose a better path per backend.
- Direct mode complicates display-agnostic presentation.
- Overlay drawing can force full copies without a clean invalidation model.
- Global state makes it hard to measure one piece in isolation.

Target performance expectation:

- Keep the indexed presentation frame compact.
- Let backends scale when they can.
- Use CPU fallback only when needed.
- Reuse all large buffers.
- Preserve X11 SHM behavior until a replacement is measured.

## Migration Constraints

Do not remove global aliases too early.

- Many modules still call `cthughaDisplay`, `displayDevice`, `screen`, `now`,
  and `deltaT`.
- The safe path is to introduce owned objects first, then shrink global usage.

Do not redesign every screen entry before the renderer context exists.

- The current constructor is not beautiful, but it is serviceable.
- Better catalog ergonomics should wait until the final rendering metadata is
  better known.

Do not move X11 resource management into generic code.

- X11 has real platform complexity: SHM, pixmaps, root window, panels,
  colormaps, and expose/resize handling.
- The target is not to simplify those details away. The target is to keep them
  behind the backend boundary.

Preserve visual behavior while extracting.

- `Source`, `scalex`, `scaley`, and the classic mirror effects are especially
  useful characterization cases.
- Small-frame pixel tests should protect them before broader movement.
