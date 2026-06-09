# TODO

## Near-Term Plan

1. Add diagnostic visuals.
   - Add controlled ~~display modes~~, ~~waves~~, flames, and translations that make visual bugs
     easy to see.
   - Useful diagnostics: ~~checkerboard, border frame~~, horizontal and vertical ramps,
     ~~quadrant markers~~, ~~fixed/no-op flame~~, ~~fixed wave~~, and hard wipe translation.
   - ~~Make these selectable through the existing option system rather than as one-off
     debug hacks.~~

2. ~~Fix stale frame regions in display transitions.
   - Reproduce and fix the partial frame left behind when transitioning to `zick`,
     `bent`, `plate`, and `hfield`.
   - First suspects:
     - `prepare_3d()` in `src/display.cc` clears only `BUFF_WIDTH` bytes per row even
       though `hfield`, `bent`, and `plate` are `xy(2, 2)` displays.
     - `screen_zick()` writes only the base-width region and has suspicious clearing
       around shifted rows.
   - Preserve visual behavior except for the obvious stale-region bug.~~

3. Reinforce the visual-engine/display seam.
   - Mostly done: make buffer dimensions and ownership clearer. `CthughaBuffer`
     now owns visible active/passive indexed pixels plus named hidden vertical
     border rows for flame feedback; it no longer choreographs visual startup.
   - Add shared helpers for clearing the full display surface and/or base display region.
   - Document which screen functions produce `BUFF_WIDTH x BUFF_HEIGHT` content and rely
     on mirroring, and which produce the full `2 * BUFF_WIDTH x 2 * BUFF_HEIGHT` surface.
   - Leave the classic `2 * BUFF_WIDTH x 2 * BUFF_HEIGHT` display-surface coupling intact
     until the future target display architecture is chosen; then split engine buffer,
     presentation surface, and window/output size deliberately.
   - Add debug assertions where they can catch invalid writes or bad assumptions.

4. Improve sound diagnostics and file playback.
   - Add better deterministic sound test fixtures than `-x` random noise, with controlled
     intensity ramps and attack/fire events.
   - Keep file playback output decoupled from visual-analysis sampling: read/decode into
     `AudioBuffer`, let `AudioOutput` keep the server/device buffer fed, and feed only the
     required visual window into Cthugha.
   - Add tests around MP3, WAV, and raw PCM playback so each format produces PCM for the
     same buffer/output/frame pipeline.
   - Document `--play FILE --silent` as the preferred fixture path for analysis-only
     runs.

5. ~~Revisit automatic option-change timing.
   - Compare the feel of the three current/reference behaviors:
     - DOS Cthugha 5.3 keeps a visual combination for `min_time + rand() % rand_time`
       frames. Defaults are `200 + rand() % 750`, roughly 3-14 seconds at VGA 70 Hz
       or 3-16 seconds at 60 Hz.
     - CthughaNix defaults to a time-based `AutoChanger`: `min-time=500` and
       `random-time=1000`, meaning 5-15 seconds before a full unlocked `EffectControl`
       change, unless audio/silence triggers earlier. `--little` changes only one
       option.
     - `cthugha-js` currently rolls once per second and may change one category each
       tick, so visible changes can happen much more often; any individual category
       such as wave/table/flame changes about every 7 seconds on average.
   - Decide whether JS should default closer to DOS/Nix pacing: slower dwell, less
     per-second churn, and possibly a full-combination change after a randomized wait.
   - Preserve useful controls: global lock, per-option locks, randomize-one,
     randomize-all, and manual next/previous controls.~~

6. Explore audio-driven option changes.
   - Treat this as a musical attention model, not just a beat detector: change when the
     audio suggests the current visual state has earned a turn.
   - Compare DOS 5.3 `peaknoise` with CthughaNix `fire`:
     - DOS checks whether the min/max audio range exceeds `peaklevel`; after
       `peakframes` such hits, it exits the current dwell early.
     - Nix accumulates rising RMS amplitude into `attackLevel`, emits `fire` when the
       amplitude starts to decline, accumulates `cumulativeFireLevel`, then changes when that
       crosses `cumulative-fire-level`.
   - Candidate triggers and policies:
     - Onset/attack energy for kick, snare, sharp entrances, and drops.
     - Sustained loudness for intense sections, normalized enough to avoid constant
       triggering on compressed music.
     - Spectral flux: compare FFT magnitudes frame-to-frame to catch cymbals, chord
       changes, new instruments, and timbral shifts.
     - Band-specific triggers: low band for flame/display warps, mid band for wave
       changes, high band for palette/table/color cycling.
     - Novelty/section detection using rolling audio summaries, with larger changes
       for verse/chorus/drop transitions.
     - Tempo-aware cooldowns so changes can land on rough 4/8/16-beat boundaries.
     - Entropy/texture: sparse audio favors stable options; dense/noisy audio raises
       change probability or selects more chaotic options.
     - Silence and re-entry: freeze/message during silence, then trigger a scene change
       when sound returns.
     - Adaptive boredom: if audio is steady, fall back to slow timed changes; if audio is
       active, let audio events advance options sooner.
   - A promising hybrid: base dwell of 5-15 seconds, early audio-triggered changes only
     after a short cooldown, small events change one option, strong section events change
     everything, and per-option locks are always respected.

7. Stabilize image behavior.
   - ~~Verify loading from `resources/img/`, clipping, centering, and palette selection.~~
   - Add PNG loader coverage.
   - Review visual catalog/init return-code behavior: `Application` now owns
     visual catalog startup, but several `init_*` functions still always return
     `0`. Decide whether those functions should be `void`, or whether startup
     should have real fatal error propagation.
   - ~~Keep PCX loading working as classic content while preparing for a modern image path.~~

8. Add a modern image loader.
   - ~~Add a small image-loader abstraction, with PNG as the first supported modern format.~~
   - ~~Keep the internal indexed-buffer behavior intact initially.~~
   - Define transparency semantics deliberately: alpha mask, alpha blend into indexed
     output, or separate overlay layer.

9. Modernize the build.
   - ~~Add a contemporary CMake build path that can build `xcthugha`, benchmarks,
     and future modern display targets.~~
   - Promote the CMake build as the documented reference path once packaging and
     install behavior are comfortable.
   - Keep removed legacy frontend paths out of active build/configuration surfaces.
   - Keep old build files around until the replacement build proves itself.

10. Add a modern display target.
   - Prefer SDL2 or SDL3 as the first modern frontend.
   - Preserve the core contract: the engine produces an indexed 8-bit buffer plus a
     palette; the frontend presents it.
   - Done: explicit `IndexedFrame` handoff from the video filterchain to display
     presentation, carrying `pixels`, `width`, `height`, `pitch`, and `FramePalette`.
   - Done: `CthughaDisplay` consumes `IndexedFrame` for presentation source pixels,
     geometry, pitch, and palette synchronization.
   - Done: presentation scratch buffers are allocated/reallocated lazily from
     incoming frame geometry instead of being sized from the global/current
     Cthugha buffer in the display constructor.
   - Done: introduced `IndexedDisplayFrame` as the display-agnostic indexed
     output of the selected screen/presentation effect; X11 now sizes its
     presentation path from that object rather than from hardcoded source-frame
     multiples.
   - Done: registered focused CTest coverage for `IndexedDisplayFrame` geometry,
     row pitch, capacity reuse, and the current `ScreenEntry` 2x compatibility
     output-size helper.
   - Keep X11 `DM_direct` as a legacy/backend optimization, not as part of the new
     shared display contract. The clean path should render screen transforms into an
     indexed presentation buffer, then palette-expand or upload into the backend's
     native surface.
   - Define the new frontend contract explicitly so buffer size, presentation-surface size,
     and actual window size can vary independently where useful.
   - Keep simulation size and display size separable: high-resolution windows should be
     able to present lower-resolution indexed simulations cleanly, and high-resolution
     simulations should not be forced to inherit old 320x240 effect tuning.
   - Use SDL to cover Wayland/X11/fullscreen/input for modern users before considering a
     native Wayland implementation.

11. Refactor the internal video engine filterchain.
   - Keep this scoped to internal visual-buffer operations, not audio source selection or
     display presentation.
   - Target responsibilities:
     - The visual engine knows nothing about the audio source or PCM file/device format.
     - The visual engine knows nothing about the display backend or display pixel format.
     - `CthughaBuffer` owns long-lived internal indexed buffers, visible dimensions,
       hidden vertical border storage, active/passive buffers, and swap/pixel access.
       It currently has no separate pitch/stride; reintroduce stride only if side
       padding or non-tightly-packed internal rows become real.
     - `CthughaBuffer` should be state, not choreography; it should not run or write
       itself.
     - The engine consumes processed wave data through a stable per-frame context:
       raw `AudioFrame`, FFT/spectrum data, future frequency bands/features, and
       `AcousticContext`.
   - Current video filterchain state:
     - `AudioVisualBridge` still owns audio processing/analyzer/autochanger work before
       visual mutation.
     - `ImageStage`
     - `BorderStage`
     - `FlameStage`
     - `TranslateStage`
     - `WaveStage`
     - `TextStage`
     - `FrameCommitStage`
     - `PaletteStage`
     - `FlashlightStage`
     - `IndexedFrameStage`
   - `VideoDirector` updates stage filters with the selected image, flame,
     general-flame value, translation table, wave config, border mode, palette
     target, and flashlight mode.
   - `VideoFilterchain::run()` passes one explicit `VideoFrame` through each
     enabled filter. The frame carries the current `CthughaBuffer`,
     `VideoFrameContext`, and display `FramePalette`.
   - The display path receives an explicit `IndexedFrame` from the final filterchain
     stage; presentation source pixels, geometry, pitch, and palette now flow through
     that frame.
   - The screen/presentation layer now has an explicit `IndexedDisplayFrame`
     destination. Most classic `ScreenEntry` choices still request the
     historical `2 * sourceWidth` by `2 * sourceHeight` output size through a
     compatibility helper, but that policy is no longer buried in backend
     allocation.
   - The screen/presentation catalog now lives in `Screen.h`/`Screen.cc`, with
     typed entries, catalog count, lookup by index, and the selected `screen`
     `EffectControl` separated from the display coordinator.
   - Added a `Source` screen entry that writes a source-sized 1x
     `IndexedDisplayFrame`, letting the display path scale final output instead
     of forcing every presentation effect through the historical 2x surface.
   - Updated `scalex` and `scaley` so their screen renderers no longer perform
     plain indexed-pixel doubling; they now mirror one axis in the presentation
     frame and rely on display scaling for the other axis.
   - Remaining display handoff work:
     - Add focused tests around screen transforms reading padded `IndexedFrame::pitch`
       and writing changed `IndexedDisplayFrame` geometries.
     - Make the presentation path consistently honor `IndexedFrame::pitch` when
       copying or transforming source rows.
     - Move any artistic screen transforms that mutate indexed pixels into video filters.
     - Keep backend-specific memory layout knowledge inside display backends.
   - Treat classic `screen` functions carefully:
     - If a screen function mutates the internal indexed frame as an artistic transform,
       it belongs in the video filterchain.
     - If it copies/converts into `cthughaDisplay->buffer` or knows display memory layout,
       it belongs in the display/presentation layer, not the internal visual engine.
   - Next practical slice:
     - Classify the classic `screen` functions into artistic transforms,
       presentation transforms, and backend/display-memory transforms.
     - Add focused tests around director-owned stage sequencing, stage modes, and
       frame commit behavior.
   - Audit resolution-sensitive visual effects, especially flames. Several effects were
     tuned around classic low-resolution buffers; at higher native buffer sizes, per-frame
     cooling, diffusion, lift, blur, and travel distances cover a smaller fraction of the
     image, so flames can fade before traversing enough of the display. Decide whether to
     keep those effects at a fixed simulation resolution and scale them during
     presentation, or make effect-local distances/decay rates resolution-aware.

## Short-Term Extras

- Add a frame-dump or headless regression mode that renders selected diagnostics to image
  files.
- Add AddressSanitizer and UBSan build options early.
- Add a few deterministic test scenarios for display transitions.
- Load/generate all translation tables in the background after startup, staging each table
  invisibly and activating cached tables only once complete.
- Rationalize ini file naming: accept unprefixed keys like `lock: yes` in addition to
  `cthugha.lock: yes`, warn on likely misspellings such as `chuthga.*`, and decide
  whether future auto-written ini files should keep the old prefix for compatibility.
- Document `xcthugha` as the current reference target.
- Update Help text to include the currently mapped keys
- Update help functionality:
```--list-palettes (for each map, show index, name, sets)
--list-palette-sets (for each set, show name)
--list-palettes=$set_name (for each palette in a set, show name, sets)
--list-waves (index, name)
--list-flames (index, name)
--list-translations (index, name)
--list-displays (index, name)
--list-images (index, filename)
--list-borders (index)
--list-tables (index)
--list-sound-processing (index)```
