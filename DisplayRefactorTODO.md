# Display Refactor TODO

## Intent

Move from the current global, X11-centered display path to the target
architecture in `DisplayTargetArchitecture.md`.

This plan deliberately allows a few temporary adapters. Some steps may add a
thin compatibility layer before removing old code. That is acceptable when it
lets us write tests first and preserve visual behavior.

## Guiding Rules

- Keep `IndexedFrame` as the source handoff from the video filterchain.
- Keep `IndexedDisplayFrame` as the display-agnostic presentation output.
- Do not make X11 depend on screen selection.
- Do not make screen renderers depend on X11 or `DisplayDevice`.
- Avoid per-frame allocation in new code.
- Add tests before changing hot pixel behavior.
- Keep global aliases until owned objects have a working path.
- Prefer one small extraction with tests over one sweeping rewrite.

## Phase 0: Characterize Current Behavior

Status: not started.

Goal:

- Lock down the behavior we are about to move.

Work:

- Add tiny-frame fixtures for indexed frames.
- Add helpers to compare indexed output rows.
- Identify expected output for:
  - `Source`
  - `Up`
  - `Down`
  - `scalex`
  - `scaley`
  - at least one quadrant/mirror classic mode
- Include padded source pitch in tests.

Tests:

- `ScreenRendererCharacterizationTest`
  - Uses a 4x3 or 5x4 source frame with distinct byte values.
  - Uses source pitch greater than width.
  - Verifies exact destination pixels.
- `IndexedDisplayFrameTest`
  - Already exists; extend only if new geometry behavior appears.

Notes:

- If the current functions are too global-bound to test directly, create a
  temporary test harness or adapter. That adapter can be deleted later.

## Phase 1: Add Display Geometry Types

Status: not started.

Goal:

- Stop encoding viewport decisions in globals and macros.

Work:

- Add small data types:
  - `PixelSize`
  - `PixelRect`
  - `DisplayViewport`
  - `ScaleMode`
- Keep names modest. If existing `xy` remains useful, use it where practical.
- Add a `ViewportPolicy` that calculates destination rectangles from:
  - frame size
  - output/window size
  - zoom mode
  - aspect policy

Tests:

- `ViewportPolicyTest`
  - Source-sized output centered in a larger window.
  - Fit-to-window scaling.
  - Fixed 1x and 2x zoom.
  - Oversized fixed zoom reduced or rejected according to current behavior.
  - Odd window sizes and non-even offsets.

Verification:

- No visual behavior change yet.
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`

## Phase 2: Introduce ScreenRenderContext

Status: not started.

Goal:

- Give screen renderers explicit inputs and outputs.

Work:

- Add `ScreenRenderContext` with:
  - `const IndexedFrame& source`
  - `IndexedDisplayFrame& destination`
  - row accessors for source and destination
  - source width, height, pitch
  - destination width, height, pitch
  - frame timing if needed
  - optional selection controller for retry/change requests
- Add context helpers matching current renderer needs:
  - `sourcePixels()`
  - `sourceLine(y)`
  - `destinationPixels()`
  - `destinationLine(y)`

Temporary compatibility option:

- Add a transitional `currentScreenRenderContext` used only by old renderer
  helper functions.
- Change the helpers in `display.cc` to prefer the context and fall back to
  `cthughaDisplay`.
- This is a controlled step backwards: it introduces a temporary global so tests
  can exercise renderers before every signature changes.

Tests:

- `ScreenRenderContextTest`
  - Verifies row access honors pitch.
  - Verifies destination row access honors destination pitch.
- Re-run Phase 0 characterization through the context harness.

Verification:

- No visual behavior change.
- All current display modes still compile.

## Phase 3: Change ScreenEntry Rendering To Use Context

Status: not started.

Goal:

- Remove no-argument screen renderer execution.

Work:

- Change `ScreenEntry::Function` from:

  ```cpp
  typedef int (*Function)();
  ```

  to:

  ```cpp
  typedef int (*Function)(ScreenRenderContext&);
  ```

- Update renderer functions in `display.cc` to accept context.
- Remove helper reads from `cthughaDisplay` where practical.
- Keep `screen.change(...)` only behind a selection controller on the context.

Tests:

- `ScreenRendererCharacterizationTest`
  - Now calls renderer functions directly with explicit context.
- Test geometry-retry behavior for renderers that currently call
  `screen.change(+1, 0)`.

Verification:

- Run current unit tests.
- Manual visual smoke for several display modes because many renderer
  signatures change.

## Phase 4: Extract FrameCompletion

Status: not started.

Goal:

- Move mirror completion out of X11 presentation.

Work:

- Add `FrameCompletion` helpers for indexed frames:
  - horizontal mirror
  - vertical mirror
  - no completion
- Preserve current behavior where missing regions are mirrored based on
  filled/output geometry.
- Consider explicit completion metadata only after tests show the current shape
  is stable.

Tests:

- `FrameCompletionTest`
  - Horizontal mirror with small frames.
  - Vertical mirror with small frames.
  - Combined mirror.
  - No completion leaves bytes unchanged.
  - Destination pitch greater than width.

Verification:

- X11 code should no longer own indexed mirror behavior.

## Phase 5: Extract PresentationComposer

Status: not started.

Goal:

- Create the display-agnostic object that turns source frame plus screen choice
  into a completed `IndexedDisplayFrame`.

Work:

- Add `PresentationComposer`.
- Move `prepareIndexedDisplayFrame(...)` ownership into it.
- Move screen entry output-size calculation into it.
- Build `ScreenRenderContext`.
- Run selected `ScreenEntry`.
- Apply `FrameCompletion`.
- Attach frame palette.
- Return `const IndexedDisplayFrame&`.

Temporary compatibility option:

- Keep `CthughaDisplay` owning the `IndexedDisplayFrame` initially, but delegate
  composition to `PresentationComposer`.
- Then move ownership once behavior is stable.

Tests:

- `PresentationComposerTest`
  - Source entry returns 1x output.
  - Classic entry returns 2x output.
  - `scalex` and `scaley` output geometry matches current behavior.
  - Palette pointer is carried through.
  - Buffer capacity is reused when frame size shrinks.

Verification:

- `CthughaDisplayX11::operator()` should no longer call `screen.current()` or
  run `screen()` directly once this phase is complete.

## Phase 6: Extract ViewportPolicy Into The Display Path

Status: not started.

Goal:

- Replace `checkZoom()`, `draw_size`, and `SCREEN_OFFSET` decisions with an
  explicit viewport.

Work:

- Make `DisplayRuntime` or `CthughaDisplay` ask `ViewportPolicy` for a
  `DisplayViewport`.
- Pass `DisplayViewport` into backend presentation.
- Keep global `draw_size` updated temporarily if old X11 code still needs it.
- Replace uses of `SCREEN_OFFSET_X`, `SCREEN_OFFSET_Y`, and `SCREEN_OFFSET`
  gradually.

Tests:

- `ViewportPolicyTest` from Phase 1 should now cover production code.
- `DisplayRuntimeTest` with fake backend verifies viewport passed to backend.

Verification:

- Manual visual check:
  - Source 1x centered or scaled as expected.
  - `scalex` and `scaley` fill the display as expected.
  - Window resize still clears stale border areas.

## Phase 7: Introduce DisplayPresentation And DisplayBackend

Status: not started.

Goal:

- Give platform backends a single presentation request object.

Work:

- Add `DisplayPresentation` containing:
  - `const IndexedDisplayFrame& frame`
  - `FramePalette* palette`
  - `DisplayViewport viewport`
  - overlay commands or overlay reference
  - flags such as full-copy-needed if still required
- Add `DisplayBackend` interface:
  - `processEvents()`
  - `outputSize()`
  - `present(const DisplayPresentation&)`
  - optional `setTitle/status` hooks if needed
- Implement an X11 adapter around current `DisplayDeviceX11` behavior.

Temporary compatibility option:

- `DisplayBackendX11` may internally call old `DisplayDeviceX11` methods while
  the backend is being cut over.

Tests:

- `DisplayRuntimeTest`
  - Fake backend records the last `DisplayPresentation`.
  - Runtime passes composed frame and viewport.
  - Runtime calls backend event processing.

Verification:

- `Application` can talk to `DisplayRuntime` without direct `displayDevice`
  calls for frame presentation.

## Phase 8: Move Palette Expansion Into Backend Or PixelTransfer

Status: not started.

Goal:

- Remove palette expansion from display coordination.

Work:

- Add `PixelTransfer` CPU fallback:
  - indexed to 8-bit copy
  - indexed to 16-bit native
  - indexed to 24-bit native
  - indexed to 32-bit native
  - nearest-neighbor copy/scale as needed
- Move X11 lookup-table logic behind X11 backend or `PixelTransfer`.
- Preserve existing lookup table performance for true-color X11.

Tests:

- `PixelTransferTest`
  - Palette expansion for representative byte orders and bytes-per-pixel.
  - Pitch handling for source and destination.
  - 1x copy.
  - 2x nearest-neighbor scale if CPU fallback keeps that mode.

Verification:

- X11 visual smoke in 8-bit/mapped/true-color modes where available.

## Phase 9: Separate Overlay Collection From Backend Text Drawing

Status: not started.

Goal:

- Stop presentation code from calling `Interface::current` directly.

Work:

- Add `OverlaySource` to collect overlay commands from interface and errors.
- Add backend text rendering interface for overlay commands.
- Preserve terminal text mode.
- Preserve palette darkening and `textOnScreen` behavior, but make it explicit
  in overlay or presentation flags.

Tests:

- `OverlaySourceTest`
  - Fake interface emits expected text commands.
  - Error overlay emits expected commands.
- `DisplayRuntimeTest`
  - Runtime includes overlay commands in presentation.
  - No overlay produces no text/full-copy flags.

Verification:

- Help/list/error text still appears.
- Text disappearing still clears stale pixels.

## Phase 10: Move Frame Clock And Pacing Out Of CthughaDisplay

Status: not started.

Goal:

- Stop display presentation from owning timing used by the whole runtime.

Work:

- Add `FrameClock`.
- Add `FramePacer`.
- Keep `now` and `deltaT` as temporary aliases updated from `FrameClock`.
- Move `maxFPS` sleep logic into `FramePacer`.
- Keep `CthughaDisplay::status()` behavior available through `DisplayRuntime`
  or a status object.

Tests:

- `FrameClockTest`
  - Deterministic fake time source.
  - Correct `deltaT`.
- `FramePacerTest`
  - Fake sleep function receives expected sleep duration.
  - `maxFPS=0` does not sleep.

Verification:

- Audio, AutoChanger, filters, and display still observe the same frame time.

## Phase 11: Move Display Ownership Into Application

Status: not started.

Goal:

- Replace global construction with owned runtime objects.

Work:

- Change display factories to return objects:
  - `std::unique_ptr<DisplayBackend> createDisplayBackend(...)`
  - `std::unique_ptr<DisplayRuntime> createDisplayRuntime(...)`
- Store them as `Application` members.
- Keep `displayDevice` and `cthughaDisplay` as temporary aliases if necessary.
- Remove aliases once all callers use injected references or narrower
  interfaces.

Tests:

- `ApplicationLifecycleTest` if practical:
  - Fake backend initializes and shuts down in order.
  - Shutdown is idempotent.
- Existing application startup smoke.

Verification:

- No double-delete or alias lifetime issue.
- Suspend/resume callbacks still work.

## Phase 12: Retire Or Isolate Direct Mode

Status: not started.

Goal:

- Decide what to do with `DM_direct`.

Work:

- Measure whether direct drawing still matters.
- If it matters, keep it as an X11 backend optimization hidden behind
  `DisplayBackendX11`.
- If it does not matter, remove it and always compose indexed presentation
  before backend conversion.

Tests:

- Existing presentation tests should be unchanged.
- X11 backend tests or manual smoke should cover whichever path remains.

Verification:

- No screen renderer writes directly to native display memory outside backend
  control.

## Phase 13: Remove Transitional Globals And Adapters

Status: not started.

Goal:

- Finish the IoC migration.

Work:

- Remove transitional `currentScreenRenderContext`.
- Remove `cthughaDisplay` reads from screen renderers.
- Remove `displayDevice` reads from display runtime code.
- Remove `draw_size`, `disp_size`, `bypp`, `bytes_per_line`, and `draw_mode`
  from generic code. Keep backend-owned equivalents where needed.
- Remove `SCREEN_OFFSET` macros from generic code.
- Ensure `Application` owns the display object graph.

Tests:

- Full unit suite.
- Header self-containment.
- Manual X11 smoke:
  - default display
  - Source
  - scalex
  - scaley
  - at least one 3-D screen
  - resize
  - overlay text
  - palette changes

Verification:

- `rg "cthughaDisplay|displayDevice|SCREEN_OFFSET|draw_size|disp_size|draw_mode|bytes_per_line|bypp" src`
  should show only expected backend or compatibility locations.

## Suggested First Implementation Slice

Start with Phase 0 and Phase 1.

Reason:

- They build test scaffolding and pure geometry behavior without forcing a big
  signature change.
- They let us learn where the awkward current assumptions actually are.
- They do not require choosing final catalog ergonomics yet.

The first commit should ideally contain only:

- Tiny-frame test helpers.
- Viewport data types.
- `ViewportPolicy` tests.
- No production display behavior change unless unavoidable.

The second commit can introduce `ScreenRenderContext` and the temporary context
adapter for screen renderer tests.

## Ongoing Verification Commands

Run these after each slice:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
git diff --check
```

When a slice touches X11 presentation, also run a short manual visual smoke
with a representative display mode and at least one audio file.
