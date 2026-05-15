# TODO

## Near-Term Plan

1. Add diagnostic visuals.
   - Add controlled display modes, waves, flames, and translations that make visual bugs
     easy to see.
   - Useful diagnostics: checkerboard, border frame, horizontal and vertical ramps,
     quadrant markers, fixed/no-op flame, fixed wave, and hard wipe translation.
   - Make these selectable through the existing option system rather than as one-off
     debug hacks.

2. Fix stale frame regions in display transitions.
   - Reproduce and fix the partial frame left behind when transitioning to `zick`,
     `bent`, `plate`, and `hfield`.
   - First suspects:
     - `prepare_3d()` in `src/display.cc` clears only `BUFF_WIDTH` bytes per row even
       though `hfield`, `bent`, and `plate` are `xy(2, 2)` displays.
     - `screen_zick()` writes only the base-width region and has suspicious clearing
       around shifted rows.
   - Preserve visual behavior except for the obvious stale-region bug.

3. Reinforce the visual-engine/display seam.
   - Make buffer dimensions and ownership clearer: base visual buffer versus full display
     buffer.
   - Add shared helpers for clearing the full display surface and/or base display region.
   - Document which screen functions produce `BUFF_WIDTH x BUFF_HEIGHT` content and rely
     on mirroring, and which produce the full `2 * BUFF_WIDTH x 2 * BUFF_HEIGHT` surface.
   - Add debug assertions where they can catch invalid writes or bad assumptions.

4. Stabilize PCX behavior.
   - Verify loading from `pcx/`, `.pcx.gz`, clipping, centering, palette selection, and
     screenshot save behavior.
   - Keep PCX working as legacy content while preparing for a modern image path.

5. Add a modern image loader.
   - Add a small image-loader abstraction, with PNG as the first supported modern format.
   - Keep the internal indexed-buffer behavior intact initially.
   - Define transparency semantics deliberately: alpha mask, alpha blend into indexed
     output, or separate overlay layer.

6. Modernize the build.
   - Add a contemporary build path that can build `xcthugha` and future modern display
     targets.
   - Quarantine or optionally disable SVGAlib and old OpenGL/GLUT paths by default.
   - Keep old build files around until the replacement build proves itself.

7. Add a modern display target.
   - Prefer SDL2 or SDL3 as the first modern frontend.
   - Preserve the core contract: the engine produces an indexed 8-bit buffer plus a
     palette; the frontend presents it.
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
