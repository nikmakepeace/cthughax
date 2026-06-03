# Display Target Architecture

## Goal

The display path should be testable without X11, SDL, Wayland, or a real window.
It should also be display-agnostic: the video filterchain and screen effects
produce indexed Cthugha pixels, while platform backends decide how to upload,
convert, scale, and present them.

The intended direction is inversion of control. The application constructs the
display runtime and injects its collaborators. Screen effects receive an
explicit render context. Backends receive a completed presentation request.
No screen effect should discover the active display through global state, and no
backend should discover the active screen effect through global state.

## High-Level Flow

```text
Application
  -> DisplayRuntime::processEvents()
  -> DisplayRuntime::beginFrame()
  -> video filterchain produces IndexedFrame
  -> DisplayRuntime::present(IndexedFrame)

DisplayRuntime::present()
  -> ScreenSelection chooses ScreenEntry
  -> PresentationComposer composes IndexedDisplayFrame
  -> ViewportPolicy computes DisplayViewport
  -> OverlaySource emits overlay draw commands
  -> DisplayBackend presents DisplayPresentation
```

The important separation is:

- The video filterchain produces an `IndexedFrame`.
- The presentation layer transforms that into an `IndexedDisplayFrame`.
- The backend presents that frame to a platform window or output.

## Objects

### Application

`Application` is the composition root.

Responsibilities:

- Parse options and initialize runtime systems.
- Construct the display runtime and all display collaborators.
- Own the main loop.
- Advance audio/video/display frame phases in order.
- Shut down display objects in reverse construction order.

Construction dependencies:

- Command-line options and config values.
- `Scene` and `SceneCommands`.
- Factories for the selected display backend.
- The selected `ScreenCatalog` and screen option state.

Messages sent:

- `displayRuntime.processEvents()`
- `displayRuntime.beginFrame()`
- `displayRuntime.present(indexedFrame)`
- `displayRuntime.observeLatency(seconds)` if latency is not internal.

Coupling:

- Coupled to `DisplayRuntime` as a facade.
- Not coupled to X11, SDL, Wayland, pixel formats, or screen-effect internals.

### DisplayRuntime

`DisplayRuntime` is the display facade used by `Application`.

Responsibilities:

- Coordinate the display-specific part of a frame.
- Own or borrow `FrameClock`, `FramePacer`, `ScreenSelection`,
  `PresentationComposer`, `ViewportPolicy`, `OverlaySource`, and
  `DisplayBackend`.
- Provide user-facing display status such as FPS and presentation latency.
- Keep display sequencing in one place.

Construction dependencies:

- `DisplayBackend&`
- `ScreenSelection&`
- `PresentationComposer&`
- `ViewportPolicy&`
- `OverlaySource*` or `OverlaySource&`
- `FrameClock&` or `FrameTiming&`
- `DisplaySettings&` or a settings snapshot provider.

Messages sent:

- To backend: `processEvents()`, `windowSize()`, `present(...)`.
- To composer: `compose(sourceFrame, selectedScreen, timing)`.
- To viewport policy: `viewportFor(frameSize, outputSize, settings)`.
- To overlay source: `collectOverlayCommands(...)`.

Coupling:

- Coupled to display abstractions.
- Not coupled to a concrete platform backend.
- Not coupled to individual screen algorithms.

### FrameClock

`FrameClock` owns the visual clock formerly exposed through `now` and `deltaT`.

Responsibilities:

- Publish current frame time and elapsed frame time.
- Provide a single timing source to audio analysis, AutoChanger, visual filters,
  screen effects, and display presentation.

Construction dependencies:

- A time source, such as a function object wrapping `getTime()`.

Messages sent:

- Usually none. Other objects read its current `FrameTiming`.

Coupling:

- Not coupled to display backends.
- May be owned by `Application` instead of `DisplayRuntime` because audio and
  video also depend on it.

### FramePacer

`FramePacer` enforces `maxFPS`.

Responsibilities:

- Decide whether to sleep at the end or beginning of a frame.
- Keep pacing policy out of presentation code.

Construction dependencies:

- `DisplaySettings` or an injected `maxFramesPerSecond` value.
- A sleep function for testability.

Messages sent:

- Calls the injected sleep function.

Coupling:

- Not coupled to backends, screen effects, or indexed frames.

### DisplaySettings

`DisplaySettings` is the display configuration view.

Responsibilities:

- Hold or expose display options such as display mode, zoom mode, scaling mode,
  text-on-terminal, fullscreen, root-window mode, MIT-SHM preference, and any
  backend-specific options.
- Provide a stable snapshot for a frame, so frame presentation does not chase
  mutable globals.

Construction dependencies:

- Existing option objects during the transition.
- Eventually a parsed configuration model.

Messages sent:

- Usually none.

Coupling:

- Platform-neutral settings should be separate from X11-specific settings.
- X11-specific settings should be injected only into the X11 backend factory.

### ScreenCatalog

`ScreenCatalog` owns the available screen/presentation effects.

Responsibilities:

- Store named `ScreenEntry` objects.
- Provide lookup by index or name.
- Expose entries to the option system.

Construction dependencies:

- The built-in screen renderer functions.

Messages sent:

- None during rendering.

Coupling:

- Coupled to the option system while `EffectControl` remains the selection
  mechanism.
- Not coupled to display backends.

### ScreenSelection

`ScreenSelection` is a small adapter around the selected screen option.

Responsibilities:

- Return the currently selected `ScreenEntry`.
- Allow a screen renderer to request another entry when the current geometry is
  unsuitable.

Construction dependencies:

- Existing `EffectControl& screen` during the transition.
- Later a cleaner selection model.

Messages sent:

- To option state: `current()`, `change(...)`, or equivalent.

Coupling:

- Coupled to selection state.
- Not coupled to X11 or native display surfaces.

### ScreenEntry

`ScreenEntry` is metadata plus one renderer.

Responsibilities:

- Name and describe a screen effect.
- Declare requested output geometry.
- Declare how much of the output the renderer fills.
- Execute the renderer through a render context.

Construction dependencies:

- Renderer function or object.
- Name, description, enabled flag.
- Geometry/completion metadata.

Messages sent:

- To renderer: `render(context)`.

Coupling:

- Coupled to `ScreenRenderContext`.
- Not coupled to `CthughaDisplay`, `DisplayDevice`, X11, or global buffers.

### ScreenRenderContext

`ScreenRenderContext` is the central IoC object for screen effects.

Responsibilities:

- Provide read-only access to the source `IndexedFrame`.
- Provide mutable access to the destination `IndexedDisplayFrame`.
- Provide source and destination geometry, pitch, and helper row accessors.
- Provide current frame timing and FPS if a screen effect needs it.
- Provide a controlled way to request another screen choice.

Construction dependencies:

- `const IndexedFrame& source`
- `IndexedDisplayFrame& destination`
- `FrameTiming`
- Optional `ScreenSelectionController&` for retry/change requests.

Messages sent:

- To selection controller only when a renderer rejects the current geometry.

Coupling:

- Not coupled to a backend.
- Not coupled to global `cthughaDisplay`.

### ScreenRenderer

`ScreenRenderer` is the function or class that actually transforms pixels.

Responsibilities:

- Read source indexed pixels.
- Write destination indexed pixels.
- Return a render result, for example success or retry with another screen.

Construction dependencies:

- Usually none.
- Some future renderers may carry precomputed tables or settings.

Messages sent:

- Only to `ScreenRenderContext`.

Coupling:

- Coupled to indexed frame shape.
- Not coupled to palette expansion, native pixels, overlays, or platform APIs.

### PresentationComposer

`PresentationComposer` owns display-agnostic presentation assembly.

Responsibilities:

- Allocate and reuse an `IndexedDisplayFrame` for the selected screen output.
- Build `ScreenRenderContext`.
- Run the selected `ScreenEntry`.
- Apply completion policy such as mirror, none, clear, repeat, or future modes.
- Attach the palette to the completed indexed display frame.
- Return a `PresentationFrame` or `IndexedDisplayFrame` ready for backend
  presentation.

Construction dependencies:

- `IndexedDisplayFrame` storage, owned internally.
- Optional `FrameCompletion` helpers.

Messages sent:

- To `ScreenEntry::render(context)`.
- To completion helpers.

Coupling:

- Coupled to screen entries and indexed frame data.
- Not coupled to platform backends.
- Not coupled to overlay or native pixel formats.

### FrameCompletion

`FrameCompletion` applies post-render completion to indexed pixels.

Responsibilities:

- Complete missing regions after a renderer fills only part of its requested
  output.
- Implement mirror, none, and future strategies like repeat, clear, clamp, or
  crop.

Construction dependencies:

- None for simple strategies.

Messages sent:

- None beyond pixel copies.

Coupling:

- Coupled to indexed destination geometry.
- Not coupled to source frames or backends.

### DisplayViewport

`DisplayViewport` is the resolved presentation geometry.

Responsibilities:

- Describe where the indexed display frame lands in the platform output.
- Carry source rectangle, destination rectangle, scale mode, and filtering.
- Replace `draw_size`, `disp_size`, and `SCREEN_OFFSET` style derived globals.

Construction dependencies:

- None. It is data.

Messages sent:

- None.

Coupling:

- Shared between viewport policy and backend.

### ViewportPolicy

`ViewportPolicy` calculates `DisplayViewport`.

Responsibilities:

- Turn frame size, output/window size, zoom mode, aspect policy, and scaling
  policy into rectangles.
- Decide letterboxing/pillarboxing.
- Decide whether a fixed zoom is possible or should be reduced.

Construction dependencies:

- `DisplaySettings` or a frame settings snapshot.

Messages sent:

- None except perhaps diagnostics.

Coupling:

- Not coupled to X11.
- Not coupled to screen renderers.

### OverlaySource

`OverlaySource` provides text and UI overlay commands.

Responsibilities:

- Ask the active interface and error system what should be drawn.
- Produce backend-neutral overlay commands such as text, color role, position,
  and justification.

Construction dependencies:

- `Interface&`
- `ErrorDisplay&` or equivalent.

Messages sent:

- To interface/error objects to gather overlay state.

Coupling:

- Coupled to UI state.
- Not coupled to X11 text drawing.

### OverlayRenderer

`OverlayRenderer` turns overlay commands into backend work.

Responsibilities:

- Render text commands onto a backend surface, texture, or terminal.
- Apply palette darkening rules if still required.

Construction dependencies:

- Backend text drawing interface.
- Font metrics.

Messages sent:

- To backend drawing primitives.

Coupling:

- Coupled to backend text capabilities.
- Not coupled to screen effects or video filters.

### DisplayBackend

`DisplayBackend` is the platform abstraction.

Responsibilities:

- Own platform resources such as windows, connections, images, textures,
  Wayland buffers, SDL renderers, or X11 SHM images.
- Process platform events.
- Report output size and pixel format.
- Present a completed indexed display frame using a viewport and palette.
- Perform or delegate palette expansion, upload, scaling, and final swap/copy.

Construction dependencies:

- Platform-specific settings.
- Scene/UI hooks only when the platform has optional panels that need them.
- Optional `PixelTransfer` CPU fallback.

Messages sent:

- To OS/platform APIs.
- To optional panel/control objects.

Coupling:

- Coupled to platform APIs.
- Not coupled to screen selection.
- Not coupled to video filterchain internals.

### PixelTransfer

`PixelTransfer` is a reusable CPU fallback for palette expansion and scaling.

Responsibilities:

- Convert indexed pixels plus palette to backend-native pixels.
- Copy or scale rows into a destination surface.
- Respect pitch on source and destination.
- Provide nearest-neighbor scaling at minimum.

Construction dependencies:

- None, or tables derived from pixel format.

Messages sent:

- None beyond memory writes.

Coupling:

- Coupled to pixel formats and memory layout.
- Not coupled to X11 calls.

Backends can bypass `PixelTransfer` when the platform can do better, for
example by uploading an 8-bit texture plus palette or by using GPU scaling.

## Construction

`Application::initialize()` should become the composition root for display:

```text
DisplaySettings settings = DisplaySettings::fromOptions(...);
FrameClock frameClock(timeSource);
FramePacer framePacer(settings, sleepFunction);

ScreenCatalog screenCatalog = makeBuiltInScreenCatalog();
ScreenSelection screenSelection(screenOption, screenCatalog);
PresentationComposer composer;
ViewportPolicy viewportPolicy(settings);
OverlaySource overlaySource(interface, errors);

std::unique_ptr<DisplayBackend> backend =
    DisplayBackendFactory::create(settings, scene, sceneCommands);

DisplayRuntime displayRuntime(
    *backend,
    screenSelection,
    composer,
    viewportPolicy,
    overlaySource,
    frameClock,
    framePacer);
```

During the transition, existing globals may remain as aliases. The target is
that object ownership lives in `Application`, not in global constructors or
factory functions that mutate global pointers.

## Message Coupling

Allowed coupling:

- `Application` knows `DisplayRuntime`.
- `DisplayRuntime` knows display abstractions.
- `PresentationComposer` knows screen abstractions and indexed frames.
- `ScreenRenderer` knows only `ScreenRenderContext`.
- `DisplayBackend` knows platform APIs and presentation data.
- `OverlaySource` knows UI state.

Avoided coupling:

- Screen renderers do not know `CthughaDisplay`.
- Screen renderers do not know `DisplayDevice`.
- Backends do not know `screen.current()`.
- Backends do not invoke `Interface::current`.
- Viewport policy does not use X11 globals.
- Video filters do not know display backends.

## Testability

The target architecture should support these tests without a window:

- Screen renderer tests with tiny indexed frames and padded pitch.
- Completion tests for mirror/none/repeat/clear behavior.
- Presentation composer tests that assert output size, pitch, palette, and pixel
  content.
- Viewport policy tests for zoom, fit, letterbox, oversized windows, and resize.
- Pixel transfer tests for 8-bit, 16-bit, 24-bit, and 32-bit conversion.
- Display runtime tests using a fake backend that records presentation requests.
- Overlay tests using fake overlay sources and fake text renderers.

Only backend integration tests should need X11, SDL, or Wayland.

## Performance Considerations

Avoid per-frame allocation.

- `PresentationComposer` should reuse `IndexedDisplayFrame` capacity.
- Backends should reuse native surfaces, textures, and upload buffers.
- Resize should be the normal reason for reallocation.

Avoid unnecessary full-frame copies.

- The ideal path is filterchain indexed frame -> presentation indexed frame ->
  backend upload/present.
- Completion should operate in-place when possible.
- Scaling should happen in the backend when the backend can do it cheaply.

Respect pitch.

- Source and destination row pitch must be explicit in every renderer and copy
  helper.
- Tests should include padded rows to prevent accidental tight-row assumptions.

Keep per-pixel virtual dispatch out of hot loops.

- Virtual calls at frame boundaries are fine.
- Pixel loops should use concrete helper functions or templates where needed.

Palette expansion is hot.

- X11 true-color modes currently use lookup tables. Keep that behavior or
  something equivalent.
- SDL/Wayland may prefer texture upload plus backend scaling.

Scaling can dominate cost.

- Scaling 320x240 to 2160p is cheap for a GPU or compositor but expensive as a
  CPU write every frame.
- `ViewportPolicy` should describe desired scaling; `DisplayBackend` should
  decide the best implementation.

Overlay can force full copies.

- Text overlays and palette darkening can invalidate partial-update strategies.
- Keep overlay invalidation explicit so backends can choose full or partial
  presentation.

## Biggest Risks

Screen effects rely on hidden global state.

- Many renderers in `display.cc` currently read `cthughaDisplay`, `screen`, FPS,
  source dimensions, destination buffers, and global timing indirectly.
- This is the largest refactor risk and should be handled with characterization
  tests before changing signatures.

The legacy 2x/mirror model is embedded in behavior.

- Some effects fill a quadrant or half and expect display code to mirror.
- New metadata must preserve exact output for existing modes while allowing 1x
  or backend-scaled output.

X11 has multiple presentation paths.

- Direct, mapped, SHM image, SHM pixmap, non-SHM image, palette preview, text
  pixmap, root-window mode, and panel mode all interact with presentation.
- The backend abstraction must not erase these details too early.

Palette and text behavior are intertwined.

- Text-on-screen changes palette darkening and dirty state.
- Pulling overlay out of backend code must preserve palette updates.

Options are mutable at runtime.

- AutoChanger, interface actions, and command-line initial values can change
  display choices during runtime.
- Display settings should be read as a frame snapshot, not copied once and then
  forgotten.

Frame timing is shared beyond display.

- `now` and `deltaT` are used by audio, filters, and screen effects.
- Moving the clock out of `CthughaDisplay` must be coordinated with the rest of
  the runtime.

Direct mode may not survive unchanged.

- Drawing screen effects directly into native display memory is hostile to a
  backend-agnostic architecture.
- It may need to become an X11-only optimization behind the backend, or be
  retired after measurement.
