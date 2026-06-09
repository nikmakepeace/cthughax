# TODO

## Diagnostics And Deterministic Runs

1. Add remaining diagnostic visuals.
   - Add horizontal and vertical ramps.
   - Add a hard-wipe translation.
   - Add controlled flame cases that make resolution, decay, and feedback bugs
     easy to see.

2. Strengthen deterministic performance and regression fixtures.
   - Add deterministic audio fixtures with controlled intensity ramps,
     attacks, quiet sections, and fire events.
   - Add a frame-dump or headless regression mode that can render selected
     scenes to image files.
   - Add deterministic test scenarios for display transitions and scene-script
     playback.

## Visual Engine And Display

3. Continue isolating remaining display compatibility globals.
   - Move `disp_size`, `bypp`, `bytes_per_line`, `draw_mode`, `text_size`, and
     `fontSize` behind backend-owned geometry, pixel-format, and overlay-layout
     state where practical.
   - Keep any unavoidable compatibility state narrow and X11-specific.
   - Keep backend-specific memory layout knowledge inside display backends.

4. Finish classic screen/presentation cleanup.
   - Classify classic screen functions into artistic transforms, presentation
     transforms, and backend/display-memory transforms.
   - Move artistic transforms that mutate indexed pixels into frame filters.
   - Make the presentation path consistently honor `IndexedFrame::pitch` when
     copying or transforming source rows.
   - Add focused tests around screen transforms reading padded
     `IndexedFrame::pitch` and writing changed `IndexedDisplayFrame`
     geometries.

5. Tune high-resolution visual behavior.
   - Audit resolution-sensitive flames and translations at modern buffer sizes.
   - Decide which effects should stay at fixed simulation resolution and scale
     during presentation.
   - Make effect-local distances, decay, cooling, diffusion, lift, and blur
     resolution-aware where fixed-resolution simulation is not appropriate.

6. Keep SDL3 portable while preserving X11 compatibility.
   - Validate the SDL3 frontend on Windows.
   - Keep Linux/GNOME and macOS SDL3 behavior healthy.
   - Keep the X11 `xcthugha` target green as the compatibility path.
   - Treat X11 `DM_direct` as a backend optimization, not part of the shared
     display contract.

## Audio And Scene Changes

7. Improve sound diagnostics and playback coverage.
   - Add tests around MP3, WAV, and raw PCM playback so each format produces PCM
     for the same ingest/output/frame pipeline.
   - Document `--play FILE --silent` as the preferred fixture path for
     analysis-only runs.
   - Add trace/report coverage that makes miniaudio, Pulse, OSS, and null
     timing differences easier to compare.

8. Explore richer audio-driven scene changes.
   - Treat this as a musical attention model, not just a beat detector.
   - Compare DOS 5.3 `peaknoise` behavior with the current cumulative-fire
     policy.
   - Consider onset/attack energy, sustained loudness, spectral flux,
     band-specific triggers, novelty/section detection, tempo-aware cooldowns,
     entropy/texture, silence/re-entry, and adaptive boredom.
   - A promising hybrid: base dwell of 5-15 seconds, early audio-triggered
     changes after a short cooldown, small events change one option, strong
     section events change everything, and per-option locks are respected.

## Assets And Runtime Configuration

9. Stabilize image behavior.
   - Add PNG loader coverage.
   - Review visual catalog/init return-code behavior. Several startup helpers
     still always return `0`; decide whether they should become `void` or
     propagate fatal startup failures.
   - Define transparency semantics deliberately: alpha mask, alpha blend into
     indexed output, or separate overlay layer.

10. Rationalize ini and runtime listing UX.
    - Accept unprefixed keys like `lock: yes` in addition to
      `cthugha.lock: yes`.
    - Warn on likely misspellings such as `chuthga.*`.
    - Decide whether future auto-written ini files should keep the old prefix
      for compatibility.
    - Update help text to include the currently mapped keys.
    - Add listing commands:
      - `--list-palettes`
      - `--list-palette-sets`
      - `--list-palettes=$set_name`
      - `--list-waves`
      - `--list-flames`
      - `--list-translations`
      - `--list-displays`
      - `--list-images`
      - `--list-borders`
      - `--list-tables`
      - `--list-sound-processing`

## Build And Tooling

11. Keep modern CMake as the reference build.
    - Promote the CMake build as the documented reference path once packaging
      and install behavior are comfortable.
    - Keep removed legacy frontend paths out of active build/configuration
      surfaces.
    - Decide how long old build files should stay in the tree.

12. Add developer verification modes.
    - Add AddressSanitizer and UBSan build options.
    - Load/generate translation tables in the background after startup, staging
      each table invisibly and activating cached tables only once complete.
