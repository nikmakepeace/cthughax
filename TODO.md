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

5. Stabilize PCX behavior.
   - Verify loading from `pcx/`, `.pcx.gz`, clipping, centering, palette selection, and
     screenshot save behavior.
   - Review init/load return-code behavior: several `init_*` functions currently always
     return `0`, while `CthughaBuffer::initAll()` still checks them and calls `exit(0)`.
     Decide whether those functions should be `void`, or whether startup should have real
     fatal error propagation.
   - Keep PCX working as legacy content while preparing for a modern image path.

6. Add a modern image loader.
   - Add a small image-loader abstraction, with PNG as the first supported modern format.
   - Keep the internal indexed-buffer behavior intact initially.
   - Define transparency semantics deliberately: alpha mask, alpha blend into indexed
     output, or separate overlay layer.

7. Modernize the build.
   - Add a contemporary build path that can build `xcthugha` and future modern display
     targets.
   - Quarantine or optionally disable SVGAlib and old OpenGL/GLUT paths by default.
   - Keep old build files around until the replacement build proves itself.

8. Add a modern display target.
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
- Document `xcthugha` as the current reference target and SVGAlib as archival.
- Revisit `glcthugha` later; it depends on old GLUT/OpenGL assumptions and likely should
  not block the SDL path.
