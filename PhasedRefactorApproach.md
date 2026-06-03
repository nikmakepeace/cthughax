# Phased Display Refactor Approach

This document describes a safe migration path from the current global,
X11-centered display path to the target architecture in
`DisplayTargetArchitecture.md`.

The plan is intentionally conservative. The current display path is fast enough
to be useful, and MIT-SHM is a major performance win on macOS/XQuartz. The
refactor should preserve that behavior while moving responsibility out of
globals and into explicit, testable objects.

## Current Shape To Respect

The current path is already partly modernized:

- The video filterchain publishes an `IndexedFrame`.
- `IndexedDisplayFrame` owns the display-agnostic indexed presentation buffer.
- `Screen.h` and `Screen.cc` hold the screen catalog.
- `Application::runFrame()` already has a clear handoff from video to display.

The unsafe center is still `CthughaDisplayX11::operator()`. It currently:

- selects the screen effect through the global `screen`;
- allocates/resizes the indexed display frame;
- sets up backend draw memory;
- exposes buffers through global `cthughaDisplay`;
- runs screen renderers;
- mirrors incomplete output regions;
- expands the indexed palette to native pixels;
- applies zoom and border clearing;
- calls `Interface::current` and `errors`;
- calls X11 `postDraw()`.

The refactor succeeds only if that function shrinks in small, verified steps.

## Non-Negotiable Safety Rules

- Keep `IndexedFrame` as the source handoff from the video filterchain.
- Keep `IndexedDisplayFrame` as the display-agnostic presentation buffer.
- Preserve X11 MIT-SHM and lookup-table performance until a replacement is
  measured.
- Avoid per-frame allocation in new code.
- Respect source and destination pitch in every pixel helper.
- Do not remove `cthughaDisplay`, `displayDevice`, `screen`, `now`, or `deltaT`
  aliases until owned replacements are live and callers have migrated.
- Do not make X11 responsible for screen selection after composition is
  extracted.
- Do not make screen renderers know about X11, `DisplayDevice`, or native pixel
  formats.
- Add characterization before changing hot pixel behavior.
- Every temporary compatibility adapter must have a named removal phase.

## Verification Baseline

Before relying on tests, wire the existing test directory into the normal CMake
build. At the moment the repository has `tests/CMakeLists.txt`, but a default
configure can report no CTest tests. The first implementation slice should make
CTest useful.

## TDD Commit Discipline

Each implementation slice should use a strict red/green commit pattern.

1. Write the test first.
   - The red commit should contain tests and test-only infrastructure.
   - It may include build-system wiring needed to compile and run the test.
   - It should not include production behavior changes.
   - Prefer a test failure over a compile failure once the test target exists.

2. Commit the failing test.
   - Record the exact failing command and failure mode.
   - If the test cannot be made to fail for the intended reason, stop and fix
     the test design before touching production code.

3. Make the smallest production change that passes the committed test.
   - Do not change expected values in the green step.
   - Do not soften assertions to match the implementation.
   - If the red test turns out to be wrong, revert or amend the red commit and
     re-run the red step rather than drifting test and code together.

4. Commit the passing implementation.
   - The green commit should contain production code and only unavoidable
     test-harness adjustments.
   - Any test expectation change in this commit needs an explicit explanation.

5. Squash the red and green commits only after review.
   - Before squashing, inspect both commits separately.
   - Confirm the red commit fails without the green commit.
   - Confirm the squashed commit passes the full verification commands.

This is meant to prevent tests and production code from co-evolving until they
agree by accident. The tests should describe the intended behavior before the
implementation is allowed to adapt.

Recommended commands after each implementation slice:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
git diff --check
```

When a slice touches X11 presentation, also run a manual visual smoke:

```sh
build/src/xcthugha --mit-shm -D 1 --play sander.mp3
build/src/xcthugha --no-mit-shm -D 1 --play sander.mp3
```

Capture timing around risky X11 slices and compare:

- `shm-level` remains as expected.
- X11 `post-draw-ms` does not regress.
- Palette changes are visible on the first frame after the change.
- Resize clears stale border pixels.
- Overlay text appears and disappears cleanly.

## Phase 0: Make Tests And Baselines Usable - DONE

### Goal

Make the refactor measurable before moving code.

### Work

- Add a `CTH_BUILD_TESTS` CMake option or otherwise include `tests/` in normal
  developer builds.
- Keep the current simple assert-based unit-test style unless there is a strong
  reason to introduce a framework.
- Add tiny indexed-frame fixture helpers:
  - distinct byte values;
  - source pitch greater than width;
  - destination pitch greater than width;
  - row comparison helpers with readable failure output if practical.
- Capture current manual smoke expectations for:
  - default X11 path;
  - MIT-SHM path;
  - `Source`;
  - `scalex`;
  - `scaley`;
  - one classic mirror/quadrant mode;
  - overlay text.
- Keep the performance logs from the current and MIT-SHM paths as reference
  evidence, not as formal tests.

### Tests

- Existing `indexed_display_frame_test` runs through CTest.
- New test helper code compiles without X11.

### Exit Gate

- `ctest --test-dir build --output-on-failure` runs at least the existing unit
  test.
- No production display behavior changes.

## Phase 1: Characterize Screen Output - DONE

### Goal

Lock down renderer behavior before changing renderer signatures.

### Work

- Add `ScreenRendererCharacterizationTest`.
- Exercise current renderer behavior through the smallest possible harness.
- If direct calls are too global-bound, create a test-only or transitional
  harness that installs the minimum required display context.
- Characterize exact indexed output for:
  - `Source`;
  - `Up`;
  - `Down`;
  - `scalex`;
  - `scaley`;
  - one classic quadrant/mirror mode.
- Include padded source pitch and padded destination pitch cases.
- Record geometry retry behavior for renderers that currently call
  `screen.change(...)`.

### Tests

- `ScreenRendererCharacterizationTest`.
- Existing `IndexedDisplayFrameTest`.

### Exit Gate

- Current behavior is testable without X11.
- Tests fail if rows are treated as tightly packed when pitch says otherwise.
- No renderer signature changes yet.

## Phase 2: Introduce `ScreenRenderContext` As A Shim - DONE

### Goal

Give renderers an explicit context without forcing every renderer to change in
one commit.

### Work

- Add `ScreenRenderContext` with:
  - `const IndexedFrame& source`;
  - `IndexedDisplayFrame& destination`;
  - source and destination width, height, and pitch;
  - `sourceLine(y)` and `destinationLine(y)`;
  - frame timing values copied from existing `now`, `deltaT`, and `fps`;
  - optional selection controller for retry/change requests.
- Add a temporary `currentScreenRenderContext` compatibility adapter.
- Change old helper functions in `display.cc` to prefer the context and fall
  back to `cthughaDisplay`.
- Keep the adapter narrow and documented as transitional.

### Tests

- `ScreenRenderContextTest`.
- Phase 1 characterization tests run through the context harness where possible.

### Exit Gate

- Existing X11 runtime still works through the fallback path.
- Tests can exercise at least some screen helpers without `cthughaDisplay`.
- The temporary adapter has an explicit removal target in Phase 13.

## Phase 3: Convert `ScreenEntry` And Renderers To Context - DONE

### Goal

Remove no-argument renderer execution.

### Work

- Change `ScreenEntry::Function` from `int (*)()` to
  `int (*)(ScreenRenderContext&)`.
- Update screen renderer implementations in `src/display.cc`.
- Replace direct `cthughaDisplay->buffer` and `bufferWidth` usage with context
  destination accessors.
- Replace source access helpers with context source accessors.
- Route `screen.change(...)` through the context selection controller.
- Keep global `screen` as the selection state for now.

### Tests

- `ScreenRendererCharacterizationTest` calls renderer functions with explicit
  context.
- Geometry retry behavior is covered.

### Exit Gate

- `display.cc` renderers no longer require global `cthughaDisplay` for normal
  rendering.
- X11 visual output is unchanged for characterized modes.
- The old no-argument `ScreenEntry` execution path is gone or reduced to a
  compatibility wrapper used only by legacy code.

## Phase 4: Extract `FrameCompletion` - DONE

### Goal

Move mirror completion out of X11 presentation.

### Work

- Add `FrameCompletion` helpers for indexed frames:
  - horizontal mirror;
  - vertical mirror;
  - combined mirror;
  - no completion.
- Preserve current behavior where filled size is smaller than output size.
- Keep explicit completion metadata as a later cleanup unless tests show it is
  needed now.
- Make completion operate in-place on `IndexedDisplayFrame`.

### Tests

- `FrameCompletionTest`:
  - horizontal mirror;
  - vertical mirror;
  - combined mirror;
  - no completion;
  - padded destination pitch.

### Exit Gate

- `CthughaDisplayX11::operator()` no longer owns indexed mirror behavior.
- Characterized screen modes still match.

## Phase 5: Extract `PresentationComposer` - DONE

### Goal

Create the display-agnostic object that turns a source frame plus selected
screen into a completed `IndexedDisplayFrame`.

### Work

- Add `PresentationComposer`.
- Initially let `CthughaDisplay` keep owning the `IndexedDisplayFrame`; pass it
  into the composer.
- Move these responsibilities into the composer:
  - selected screen output-size calculation;
  - indexed display-frame resize/reuse;
  - `ScreenRenderContext` construction;
  - selected renderer execution;
  - `FrameCompletion`;
  - palette propagation.
- Keep palette expansion, zoom, overlays, and X11 `postDraw()` outside the
  composer.

### Tests

- `PresentationComposerTest`:
  - source-sized output;
  - classic 2x output;
  - `scalex` and `scaley` geometry;
  - palette pointer propagation;
  - capacity reuse when frame size shrinks;
  - geometry retry path.

### Exit Gate

- `CthughaDisplayX11::operator()` no longer calls `screen()` directly.
- X11 receives a completed indexed frame from the composer.
- No backend API changes yet.

## Phase 6: Add Display Geometry Types And `ViewportPolicy` - DONE

### Goal

Replace implicit viewport calculations with explicit data, but only after
indexed composition is stable.

### Work

- Add small geometry types:
  - `PixelSize`;
  - `PixelRect`;
  - `DisplayViewport`;
  - `ScaleMode`.
- Add `ViewportPolicy` as pure logic.
- Model current `checkZoom()`, `draw_size`, `disp_size`, and `SCREEN_OFFSET`
  behavior in tests.
- Do not remove legacy globals yet.

### Tests

- `ViewportPolicyTest`:
  - source-sized output centered in a larger window;
  - fit-to-window scaling;
  - fixed 1x and 2x zoom;
  - oversized fixed zoom reduced according to current behavior;
  - odd output sizes and non-even offsets;
  - resize cases that require border clearing.

### Exit Gate

- Viewport behavior is described by tests.
- Production code can still use legacy globals until Phase 7.

## Phase 7: Thread Viewports Through The Legacy X11 Path - DONE

### Goal

Make the existing X11 path consume an explicit viewport while preserving its
resource management and MIT-SHM behavior.

### Work

- Have `CthughaDisplayX11::operator()` ask `ViewportPolicy` for a
  `DisplayViewport`.
- Keep `draw_size`, `disp_size`, `SCREEN_OFFSET_*`, and `bytes_per_line`
  updated as compatibility aliases while old X11 helpers still need them.
- Replace use sites gradually:
  - `clearBorder()`;
  - `zoom2Screen()`;
  - X11 partial/full copy rectangles.
- Preserve current zoom reduction diagnostics.

### Tests

- Existing `ViewportPolicyTest`.
- A small adapter-level test if a fake backend can observe viewport values.

### Exit Gate

- Manual smoke confirms resize, centering, fit, fixed zoom, and border clearing.
- MIT-SHM timing does not regress relative to the current baseline.

## Phase 8: Introduce `DisplayPresentation` And `DisplayBackend` - DONE

### Goal

Give platform backends one presentation request object.

### Work

- Add `DisplayPresentation` containing:
  - `const IndexedDisplayFrame& frame`;
  - palette reference or `FramePalette*`;
  - `DisplayViewport`;
  - presentation flags such as full-copy or border-clear if still needed;
  - overlay commands only after Phase 10, or a placeholder for them.
- Add `DisplayBackend` interface:
  - `processEvents()`;
  - `outputSize()`;
  - `present(const DisplayPresentation&)`.
- Implement `DisplayBackendX11` as an adapter around current
  `DisplayDeviceX11` behavior.
- Keep X11 SHM, colormap, panel, fullscreen, root-window, and event handling
  inside the X11 backend.

### Tests

- `DisplayRuntimeTest` with a fake backend:
  - records presentation requests;
  - verifies composed frame and viewport are passed through;
  - verifies event processing is delegated.

### Exit Gate

- `Application` can call a display facade/runtime for event processing and
  presentation.
- X11 no longer discovers `screen.current()` or renderer internals.

## Phase 9: Move Palette Expansion And Pixel Transfer Behind The Backend - DONE

### Goal

Remove palette expansion from generic display coordination.

### Work

- Add `PixelTransfer` as a CPU fallback for:
  - indexed to 8-bit copy;
  - indexed to 16-bit native;
  - indexed to 24-bit native;
  - indexed to 32-bit native;
  - nearest-neighbor scaling when backend scaling is not available.
- Move X11 lookup-table expansion into `DisplayBackendX11` or an X11-specific
  transfer helper.
- Keep direct/native display memory decisions backend-owned.
- Retire the old X11 direct indexed-color path in Phase 13 so backend
  conversion always uses explicit pixel transfer.

### Tests

- `PixelTransferTest`:
  - representative byte orders;
  - source and destination pitch;
  - 1x copy;
  - 2x nearest-neighbor scaling if CPU fallback keeps it.

### Exit Gate

- Generic display runtime works only with indexed frames and presentation data.
- X11 true-color paths retain lookup-table speed.
- Manual smoke covers 8-bit/mapped/true-color paths where available.

## Phase 10: Separate Overlay Collection From Backend Text Drawing - DONE

### Goal

Stop presentation from calling `Interface::current` and `errors` directly.

### Work

- Add `OverlaySource` to collect backend-neutral overlay commands.
- Add backend text rendering hooks for overlay commands.
- Preserve terminal text mode.
- Preserve text palette darkening, stale-text clearing, and full-copy behavior
  as explicit presentation flags or overlay invalidation.
- Keep X11 panel-specific behavior inside the X11 backend.

### Tests

- `OverlaySourceTest` with fake interface/error sources.
- `DisplayRuntimeTest` verifies overlay commands and invalidation flags are
  propagated.

### Exit Gate

- `CthughaDisplayX11::operator()` or its replacement no longer invokes
  `Interface::current->display()` directly.
- Help/list/error overlays still draw and clear correctly.

## Phase 11: Extract `FrameClock` And `FramePacer` - DONE

### Goal

Move shared visual timing out of `CthughaDisplay`.

### Work

- Add `FrameClock` with an injected time source.
- Add `FramePacer` with an injected sleep function.
- Keep `now` and `deltaT` as temporary aliases updated from `FrameClock`.
- Move `maxFPS` sleep logic out of `CthughaDisplay`.
- Decide explicitly whether pacing sleeps at frame start or frame end; preserve
  old behavior first, then fix cadence in a separate behavior-changing commit if
  desired.
- Keep `CthughaDisplay::status()` behavior available through
  `DisplayRuntime` or a status object.

### Tests

- `FrameClockTest`.
- `FramePacerTest`.

### Exit Gate

- Audio analysis, AutoChanger, video filters, screen renderers, and overlays
  observe the same frame timing values as before.
- Any frame-pacing behavior change is isolated and measurable.

## Phase 12: Move Display Ownership Into `Application` - DONE

### Goal

Replace global construction with owned runtime objects.

### Work

- Change display factories to return owned objects:
  - `std::unique_ptr<DisplayBackend>`;
  - `std::unique_ptr<DisplayRuntime>`.
- Store display runtime/backend as `Application` members.
- Keep global aliases only as non-owning compatibility pointers.
- Shut down display objects in reverse construction order.
- Preserve suspend/resume lifecycle hooks.

### Tests

- `ApplicationLifecycleTest` if practical:
  - fake backend initializes and shuts down in order;
  - shutdown is idempotent.
- Existing startup/shutdown smoke.

### Exit Gate

- No double deletes.
- `Application` is the composition root for the display object graph.

## Phase 13: Retire Direct Indexed-Color Mode - DONE

### Goal

Retire the old X11 direct indexed-color path and keep all presentation through
mapped pixel transfer.

### Work

- Remove the direct indexed-color draw mode and temporary overlay mode.
- Keep PseudoColor support on the mapped 8-bit path by filling the native-pixel
  table from allocated X pixel values.
- Always pass an explicit native-pixel table to X11 pixel transfer.

### Exit Gate

- No screen renderer writes directly to native display memory outside backend
  control.
- X11 presentation no longer has a direct indexed-color draw mode.
  or tests.

## Phase 14: Remove Transitional Globals And Adapters

### Goal

Finish the inversion-of-control migration.

### Work

- Remove `currentScreenRenderContext`.
- Remove `cthughaDisplay` reads from screen renderers.
- Remove `displayDevice` reads from generic display runtime code.
- Remove `draw_size`, `disp_size`, `bypp`, `bytes_per_line`, and `draw_mode`
  from generic code. Keep backend-owned equivalents where needed.
- Remove `SCREEN_OFFSET` macros from generic code.
- Remove no-longer-needed compatibility wrappers.

### Tests

- Full unit suite.
- Header self-containment.
- Manual X11 smoke:
  - default display;
  - MIT-SHM and non-SHM;
  - `Source`;
  - `scalex`;
  - `scaley`;
  - one classic mirror/quadrant mode;
  - at least one 3-D object mode;
  - resize;
  - overlay text;
  - palette changes.

### Exit Gate

The following should appear only in expected backend or compatibility locations:

```sh
rg "cthughaDisplay|displayDevice|SCREEN_OFFSET|draw_size|disp_size|draw_mode|bytes_per_line|bypp" src
```

## Recommended First Implementation Slice

Start with Phase 0 only.

The first commit should contain:

- CMake test wiring.
- Tiny indexed-frame test helpers.
- No production display behavior change.

The second commit should characterize current screen output.

The third commit should introduce `ScreenRenderContext` as a compatibility shim.

Only after those slices should renderer signatures change.

## Why This Ordering Is Safer

The tempting path is to introduce `DisplayRuntime`, `DisplayBackend`, and
`ViewportPolicy` immediately. That would create nice names, but it would leave
the riskiest part untouched: screen renderers still pull pixels through global
`cthughaDisplay`, and X11 still owns composition.

This ordering attacks the core risk first:

1. make tests runnable;
2. characterize indexed screen behavior;
3. give renderers an explicit context;
4. extract composition;
5. only then introduce viewport/backend/runtime boundaries.

That keeps each slice small enough to verify while still moving steadily toward
the target architecture.
