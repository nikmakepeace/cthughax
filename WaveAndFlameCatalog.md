# Wave And Flame Dimension Catalog

This note inventories how built-in waves and flames tie pixels to audio,
buffer dimensions, fire events, and persistent state.

Audited against:

- `src/Wave.cc`
- `src/waves.cc`
- `src/Flame.cc`
- `src/flames.cc`
- `src/FrameGeneratorSceneBinding.cc`

## Coordinate Basics

Wave input normally comes from `FrameGeneratorContext::processedWaveData()`.
Those are internal signed 8-bit stereo samples:

```text
raw sample range: -128..127
```

Many renderers wrap this in `PreparedWaveSamples`, whose default adds `128`:

```text
prepared sample range: 0..255
```

`wave-scale` is the main fixed-pixel amplitude control for the classic
oscilloscope waves:

```text
scale0 / large  -> value >> 0
scale1 / medium -> value >> 1
scale2 / small  -> value >> 2
```

The frame geometry macros in `src/waves.cc` are:

```text
BOTTOM   = buffer.height() - 1
MID_Y    = buffer.height() / 2
MID_X    = buffer.width() / 2
LOW_LINE = buffer.height() - buffer.height() / 10
```

The normal filter order is:

```text
Image -> Border -> Flame -> Translate -> Wave -> Text -> Commit -> Palette -> Flashlight -> IndexedFrame
```

So flames generally transform feedback from previous frames and seeded pixels;
waves draw fresh audio-reactive geometry after the flame stage.

## Wave Dimension Coupling

Legend:

- Direct sample pixels: screen coordinates or lengths use raw/sample values as
  pixel offsets.
- Resolution-scaled audio: audio is normalized or used as a multiplier of a
  buffer-size-derived scale.
- Fire-derived: coordinates/counts/speeds use `runtime.fire()`, not raw waveform
  samples directly.
- Resolution/state only: geometry is driven by buffer size, object geometry,
  time, random state, or fixed grid logic. Audio may still affect colour.

| Entry | Category | Dimension coupling |
|---|---|---|
| `DotHor` | Direct sample pixels | Uses prepared samples `0..255` as vertical offsets from `LOW_LINE`, divided by `waveScale`. Horizontal position is screen scan/split by channel. |
| `DotVert` | Direct sample pixels | Uses prepared samples `0..255` as horizontal offsets from `MID_X`, divided by `waveScale`. Vertical position is screen scan. |
| `LineHor` | Direct sample pixels | Uses signed sample value, via `(sample - 128) >> waveScale`, as vertical offset from `MID_Y`. |
| `LineVert` | Direct sample pixels | Uses prepared samples `0..255` as horizontal offsets from `MID_X`, divided by `waveScale`. |
| `Spike` | Direct sample pixels | Uses `(2 * abs(rawSample)) >> waveScale` as filled spike height from the bottom, clipped to screen height. |
| `SpikeH` | Direct sample pixels | Same height formula as `Spike`, but draws only the moving outline. |
| `Walking` | Direct sample pixels plus state | Same horizontal offset family as `LineVert`, but centered around a persistent moving column. |
| `Falling` | Resolution/state only for geometry | Audio chooses palette index at fixed x positions and advancing rows. Pixel dimensions are buffer width/row state, not amplitude. |
| `Lissa` | Direct sample pixels | Uses prepared left/right samples directly as x/y coordinates with fixed historical offsets/modulo. |
| `LineX` | Direct sample pixels | Uses prepared samples as horizontal offsets; second channel is additionally shifted by 40 pixels. |
| `Light1` | Direct sample deltas | Jagged path x movement is `rawSample >> 4` style cumulative displacement, clipped to buffer width. |
| `Light2` | Direct sample deltas | Like `Light1`, but uses gentler `rawSample >> 5` style cumulative displacement. |
| `Pete0` | Direct sample pixels plus state | Samples move two drifting point clusters; coordinates wrap through the buffer. |
| `Pete1` | Resolution-mixed direct amplitude | Sine row height is based on average absolute sample energy. The average is in sample units, then clamped by `BOTTOM`; not proportional to resolution except through clipping and the width-sized sine table. |
| `Pete2` | Direct sample pixels | Uses prepared samples as horizontal offsets from `MID_X`, divided by `waveScale`; colour uses legacy sine of the sample. |
| `Fract1` | Direct sample deltas plus state | Persistent points move by half-sized differences between neighboring samples; coordinates wrap through buffer dimensions. |
| `Fract2` | Direct sample deltas plus state | Like `Fract1`, but full neighboring-sample differences make larger moves. |
| `Test` | Resolution-mixed direct amplitude | Similar to `Pete1`, but average absolute channel energy is divided by `128` and capped at `199`, so width affects accumulation and screen clips/caps affect visible height. |
| `Aaron` | Direct sample pixels | Ring/rosette offsets are `legacySine(angle) * preparedSample >> 9`; sample values directly change radius in pixels. Requires at least 256 px width and more than 128 px height. |
| `Wire1` | Resolution-scaled audio | Base object scale is `min(width,height) * 0.75`; each edge endpoint gets an independent audio-derived scale factor from its slice of the waveform. This can fracture shared vertices. |
| `Wire1dot5` | Resolution-scaled audio | Rigid object; whole model scale is `screenScale * (0.60 + 1.40 * averageAudio)`. Shape remains coherent. |
| `Wire1dot55` | Resolution-scaled audio | Same coherent audio scale as `Wire1dot5`, with a precessing rotation axis. |
| `Wire1dot6` | Mostly resolution-scaled, audio stretch | Base object uses `screenScale`; audio adds per-vertex radial stretch of `1.0..1.35`. The main size is resolution-derived, with modest audio deformation. |
| `Wire2` | Resolution/state only | Object swarm geometry is object size, buffer height/width, random placement, and time. No audio source. |
| `Wire2dot1` | Resolution/state only | Same as `Wire2`, but each copy has its own local rotation axis. No audio source. |
| `LineHLDiff` | Direct sample pixels | Uses `leftRaw - rightRaw` directly as vertical offset from `MID_Y`; no `waveScale`. Difference can reach roughly `-255..255`. |
| `Spiral` | Resolution-scaled audio metrics | Uses RMS amplitude metrics, not raw samples, scaled by `min(width,height) / 256 / 128`, then multiplied by legacy sine values. Curve radius is proportional to buffer size and current amplitude. |
| `Pyro` | Fire-derived | Launch velocity is based on `runtime.fire()` relative to rolling max fire and a screen-height-derived max velocity. |
| `Warp` | Fire-derived | Ring speed, trail count, and angular speed are based on `runtime.fire()` relative to rolling max fire; max radius comes from buffer dimensions. |
| `Laser` | Direct sample deltas plus state | Endpoint x positions advance by `abs(sample[x] - sample[x+1])` modulo buffer width. Constant full-scale samples do little; alternating samples can move endpoints very far. |
| `Corner` | Fire-derived | Fire controls random movement range and thickness; resulting corner arms extend to current point/buffer edges. Thickness is capped at 8. |
| `Jump` | Direct sample pixels plus inertia | Per-column jump energy comes from summed prepared left+right samples; visible displacement is persistent position divided by `2 + waveScale`. |
| `Sticks` | Fire-derived count, random geometry | Fire controls number of full-buffer random line segments. Segment endpoints are random across buffer dimensions. |
| `Grid` | Resolution/state only | Diagnostic grid, borders, diagonals, and corner marks based only on buffer dimensions. |
| `None` | Resolution/state only | Draws nothing. |

### Wave Families

Classic fixed-pixel amplitude waves:

```text
DotHor, DotVert, LineHor, LineVert, Spike, SpikeH, Walking, LineX,
Pete2, LineHLDiff, Jump
```

These are the ones most likely to look too small at higher buffer resolutions,
because a full-scale 8-bit sample still maps to roughly 32, 64, 128, or 255
pixels depending on formula and `waveScale`.

Resolution-scaled waves:

```text
Wire1, Wire1dot5, Wire1dot55, Wire1dot6, Spiral
```

These are closer to resolution independent in visible size. `Wire1*` still has
audio responsiveness, but the dominant scale starts from buffer dimensions.

Fire-event waves:

```text
Pyro, Warp, Corner, Sticks
```

These respond to attack/fire detection rather than raw waveform sample
amplitude.

## Flame Dimension Coupling

Flame kernels do not directly read audio samples, amplitude metrics, or fire.
Their `FrameGeneratorContext` argument is unused by the implementations. They
operate on indexed pixels already present in the active/passive frame buffers.

That means flame behaviour is indirectly audio-reactive only because earlier
frames contain wave/image/border/flashlight output that may have been driven by
audio. Their dimensions are therefore feedback-kernel dimensions: neighbor
offsets, whole-buffer scans, and hidden border rows.

| Entry | Category | Dimension coupling |
|---|---|---|
| `Clear` | Audio independent | Clears every visible pixel to palette index 0. No geometry beyond buffer dimensions. |
| `u-Sl` / Up Slow | Feedback kernel | Averages nearby pixels and a lower neighbor, writing upward. Drift is one row/frame style feedback, driven by `buffer.width()` offsets. |
| `u-Su` / Up Subtle | Feedback kernel | General four-neighbor filter with offsets below-left, below, below-right, and two rows below. Direction comes from width-based offsets. |
| `u-Fa` / Up Fast | Feedback kernel | Faster upward smear using current/lower neighbors. Direction and speed are fixed neighbor offsets, not audio. |
| `l-Sl` / Left Slow | Feedback kernel | Slow left/up drift from upper-right/current/right/lower samples. Movement is fixed one-pixel/one-row neighbor math. |
| `l-Su` / Left Subtle | Feedback kernel | General filter using right, below, below-right, and two rows below offsets. |
| `l-Fa` / Left Fast | Feedback kernel | Faster leftward/downwind smear using current, lower-right twice, and lower. |
| `r-Sl` / Right Slow | Feedback kernel | Slow right/up drift from upper-left/current/left/lower samples. |
| `r-Su` / Right Subtle | Feedback kernel | General filter using left, below-left, below, and two rows below offsets. |
| `r-Fa` / Right Fast | Feedback kernel | Faster rightward/downwind smear using current, lower-left twice, and lower. |
| `Water` | Feedback kernel | Processes top and bottom halves toward a meeting/ripple pattern. Uses buffer height/width halves and neighbor offsets. |
| `Wa-s` / Water Subtle | Feedback kernel | Same two-half shape as `Water`, but with signed-byte temporary arithmetic that changes texture/decay. |
| `Skyline` | Feedback kernel | Row-local side-neighbor smear: left/current/right/current. Flatter, horizon-like diffusion. |
| `Weird` | Feedback kernel | ORs left/current/right/lower neighbors before decay, making blockier high-contrast propagation. |
| `Zzz` | Feedback kernel | Sparse upward drift using left and lower neighbors only, then a two-sample decay table. |
| `Fade` | Audio independent decay | Uniformly darkens the previous frame by a fixed amount. No coordinate motion. |
| `GenSubt` | Configurable feedback kernel | Decodes `flame-general` into four offsets from a 3x3 neighborhood plus shared shift, then applies packed four-pixel filter. |
| `GenSlow` | Configurable feedback kernel | Same configurable offsets as `GenSubt`, but byte-by-byte. Easier to read, slower. |
| `Down` | Feedback shift | Copies the previous frame downward by one row. Top hidden border row becomes visible. |

## Flame Buffer Protocol

The flame stage is not a pure pixel transform. It participates in the
active/passive framebuffer protocol used by the whole render pipeline:

```text
active  = next frame being built
passive = previously committed/display-facing frame
```

Some flame kernels call `buffer.swapBuffers()` as their first operation. After
that swap, the old passive/display-facing frame has become active, and the
kernel edits it in place. Other kernels never swap; they read `passivePixels()`
as the previous frame and write a separate result into `activePixels()`.

No built-in flame swaps in the middle or at the end; when swapping happens, it
happens immediately on entry before sampling.

| Entry | Swap behaviour | Source -> destination | Likely reason |
|---|---|---|---|
| `Clear` | No swap | Clears active visible pixels | This is not a feedback transform. It intentionally blanks the next frame before later translate/wave drawing. |
| `u-Sl` / Up Slow | Swaps immediately | Old passive becomes active; edits active in place | Uses the previous displayed frame as the work buffer, then scan direction plus neighbor offsets create upward smear while avoiding a separate source buffer. |
| `u-Su` / Up Subtle | No swap | Passive -> active via `flame_general_subtle_filter` | Uses the generalized four-offset helper. Arbitrary/general offsets are safer as a separate previous-frame read and next-frame write. |
| `u-Fa` / Up Fast | Swaps immediately | Old passive becomes active; edits active in place | Fast in-place upward smear. The scan direction and lower-neighbor reads are part of the effect/legacy behavior. |
| `l-Sl` / Left Slow | Swaps immediately | Old passive becomes active; edits active in place | Slow in-place left/up drift. Reuses the previous displayed frame as the work buffer. |
| `l-Su` / Left Subtle | No swap | Passive -> active via `flame_general_subtle_filter` | Generalized leftward offsets use the common passive-to-active helper rather than in-place feedback. |
| `l-Fa` / Left Fast | Swaps immediately | Old passive becomes active; edits active in place | Fast in-place leftward smear using current/lower-right/lower samples. |
| `r-Sl` / Right Slow | No swap | Passive -> active | Reads upper-left/current/left/lower from the previous frame while writing the next frame. This avoids clobbering source pixels during rightward drift. |
| `r-Su` / Right Subtle | No swap | Passive -> active via `flame_general_subtle_filter` | Generalized rightward offsets use the common passive-to-active helper. |
| `r-Fa` / Right Fast | Swaps immediately | Old passive becomes active; edits active in place | Fast in-place rightward smear using current/lower-left/lower samples. |
| `Water` | No swap | Passive -> active | The top and bottom halves are computed from the previous frame into the next frame. Separate source/destination prevents the two half-scans from feeding each other mid-frame. |
| `Wa-s` / Water Subtle | No swap | Passive -> active | Same two-half water protocol as `Water`, but with signed-byte intermediate arithmetic. |
| `Skyline` | No swap | Passive -> active | Row-local side-neighbor smear. Separate source/destination keeps horizontal diffusion from reading pixels already written this frame. |
| `Weird` | No swap | Passive -> active | OR-based propagation reads left/current/right/lower from the previous frame. Separate buffers preserve the high-contrast source pattern for the whole pass. |
| `Zzz` | Swaps immediately | Old passive becomes active; edits active in place | Sparse upward drift/decay can be done in place with the chosen scan direction and two-sample decay table. |
| `Fade` | Swaps immediately | Old passive becomes active; edits active in place | Uniform decay of the previous displayed frame. No separate source buffer is needed because each pixel depends only on itself. |
| `GenSubt` | No swap | Passive -> active via `flame_general_subtle_filter` | Configurable offsets can point anywhere in the 3x3 neighborhood plus shift; a fixed in-place scan direction would not be generally safe. |
| `GenSlow` | No swap | Passive -> active via `flame_general_slow_filter` | Same configurable-offset reason as `GenSubt`, but with byte-by-byte reference-style arithmetic. |
| `Down` | No swap | Passive, offset one row upward -> active | Explicit previous-frame row shift. It reads from the passive top hidden row into the active top visible row. |

### Reorder Implications

This explains why arbitrary filterchain reordering is unsafe today:

- `Translate` also swaps active/passive before remapping passive into active.
- `Wave` draws only into active pixels.
- `FrameCommit` swaps active/passive, and `IndexedFrame` publishes passive.
- Several flames either swap immediately or overwrite active from passive.

So orders such as `Translate -> Flame` or `Wave -> Flame` are not equivalent
rearrangements. They change which buffer a later stage sees and which buffer is
eventually published. The current implementation therefore has an implicit
buffer dependency graph, even though the UI currently exposes the stages as if
they were freely reorderable peers.

### Flame Notes

Direct audio in flame kernels:

```text
none
```

Resolution dependence:

- Every flame uses `buffer.width()` to convert neighbor choices into linear
  offsets.
- Some split work by `buffer.size()` or `buffer.height()`, such as `Water`.
- Motion amount is generally one pixel or one row per frame, so the apparent
  speed is not normalized to resolution.
- Hidden top/bottom border rows can feed kernels that read above or below the
  visible buffer.

The practical consequence is that flames are mostly resolution-sensitive
feedback filters, while waves are where direct audio-to-pixel sizing usually
happens.
