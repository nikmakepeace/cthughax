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
   - Make buffer dimensions and ownership clearer: base visual buffer versus full display
     buffer.
   - Add shared helpers for clearing the full display surface and/or base display region.
   - Document which screen functions produce `BUFF_WIDTH x BUFF_HEIGHT` content and rely
     on mirroring, and which produce the full `2 * BUFF_WIDTH x 2 * BUFF_HEIGHT` surface.
   - Leave the legacy `2 * BUFF_WIDTH x 2 * BUFF_HEIGHT` display-surface coupling intact
     until the future target display architecture is chosen; then split engine buffer,
     presentation surface, and window/output size deliberately.
   - Add debug assertions where they can catch invalid writes or bad assumptions.

4. Improve sound diagnostics and file playback.
   - Add better deterministic sound test fixtures than `-x` random noise, with controlled
     intensity ramps and attack/fire events.
   - Decouple file playback output from visual-analysis sampling: read/decode into a
     buffer, write to `/dev/dsp` in backend-friendly chunks, and feed only the required
     visual window into Cthugha.
   - Document `--play FILE --silent` as the preferred fixture path for analysis-only
     runs, and `--snd-method 3` as the current smoother OSS/QEMU playback workaround.

5. ~~Revisit automatic option-change timing.
   - Compare the feel of the three current/reference behaviors:
     - DOS Cthugha 5.3 keeps a visual combination for `min_time + rand() % rand_time`
       frames. Defaults are `200 + rand() % 750`, roughly 3-14 seconds at VGA 70 Hz
       or 3-16 seconds at 60 Hz.
     - CthughaNix defaults to a time-based `AutoChanger`: `min-time=500` and
       `random-time=1000`, meaning 5-15 seconds before a full unlocked `CoreOption`
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
     - DOS checks whether the per-buffer min/max audio range exceeds `peaklevel`; after
       `peakframes` such hits, it exits the current dwell early.
     - Nix accumulates rising RMS amplitude into `attackLevel`, emits `fire` when the
       amplitude starts to decline, accumulates `fireLevel`, then changes when that
       crosses `fire-level`.
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

7. Stabilize PCX behavior.
   - ~~Verify loading from `pcx/`, `.pcx.gz`, clipping, centering, palette selection,~~ and
     screenshot save behavior.
   - Review init/load return-code behavior: several `init_*` functions currently always
     return `0`, while `CthughaBuffer::initAll()` still checks them and calls `exit(0)`.
     Decide whether those functions should be `void`, or whether startup should have real
     fatal error propagation.
   - ~~Keep PCX working as legacy content while preparing for a modern image path.~~

8. Add a modern image loader.
   - Add a small image-loader abstraction, with PNG as the first supported modern format.
   - Keep the internal indexed-buffer behavior intact initially.
   - Define transparency semantics deliberately: alpha mask, alpha blend into indexed
     output, or separate overlay layer.

9. Modernize the build.
   - Add a contemporary build path that can build `xcthugha` and future modern display
     targets.
   - Quarantine or optionally disable SVGAlib and old OpenGL/GLUT paths by default.
   - Keep old build files around until the replacement build proves itself.

10. Add a modern display target.
   - Prefer SDL2 or SDL3 as the first modern frontend.
   - Preserve the core contract: the engine produces an indexed 8-bit buffer plus a
     palette; the frontend presents it.
   - Define the new frontend contract explicitly so buffer size, presentation-surface size,
     and actual window size can vary independently where useful.
   - Use SDL to cover Wayland/X11/fullscreen/input for modern users before considering a
     native Wayland implementation.

## Short-Term Extras

- Add a frame-dump or headless regression mode that renders selected diagnostics to image
  files.
- Add AddressSanitizer and UBSan build options early.
- Add a few deterministic test scenarios for display transitions.
- Load/generate all translation tables in the background after startup, staging each table
  invisibly and activating cached tables only once complete.
- Rationalize ini file naming: accept unprefixed keys like `lock: yes` in addition to
  legacy `cthugha.lock: yes`, warn on likely misspellings such as `chuthga.*`, and decide
  whether future auto-written ini files should keep the old prefix for compatibility.
- Document `xcthugha` as the current reference target and SVGAlib as archival.
- Revisit `glcthugha` later; it depends on old GLUT/OpenGL assumptions and likely should
  not block the SDL path.
