# Explicit Dependency Refactor Plan

## Goal

Make every runtime dependency explicit. Production code should receive the
state and services it needs through constructors, method parameters, or owned
context objects. Mutable process-wide globals should disappear from normal
application code. Temporary compatibility aliases are acceptable only when they
have a named removal phase and a test that prevents new call sites.

This review covers production `src/` code. The existing display refactor has
already moved much of screen composition toward explicit `IndexedFrame`,
`ScreenRenderContext`, `PresentationComposer`, and `DisplayRuntime` handoffs.
The remaining globals are concentrated in options/effect registries, audio
facades, display backend state, UI/input state, and process services.

## Remaining Ambient Dependency Inventory

### Process Services

- Shutdown flag: `cthugha_close` in `misc.cc`, read by `Application::run()` and
  incremented by key actions, audio completion, X11 quit, credits, and resume
  failure.
- Time: `gettime()` and `getTime()` in `misc.cc`; used by frame timing, audio
  runtime, display backend tracing, error expiry, credits/help animation, and
  auto-change policy.
- Frame timing state: `now`, `deltaT`, the static `FrameClock` in
  `CthughaDisplay.cc`, and the static `FramePacer` in `Application.cc`.
- Randomness: `srand(time(0))`, `rand()`, and `Random()`; used by option
  randomization, auto-change, palette generation, waves, messages, and random
  audio.
- Logging: `cthugha_verbose` plus `CTH_*` macros; effectively a global logger.
- Shell execution: `systemf()`, used by loaders that can shell out for legacy
  behavior.

### Option And Configuration Globals

- Core options: `options_save`, `double_load`, `cthugha_verbose`.
- Audio options/config: `audioInputMode`, `soundFormat`, `soundChannels`,
  `soundSampleRate`, `soundDSPMethod`, `soundDSPFragments`,
  `soundDSPFragmentSize`, `soundDSPSync`, `soundSilent`, `audioInputLoop`,
  `dev_dsp`, `dev_mixer`, `pulse_server`, `pulse_latency_msec`,
  `audio_output_dump`, `audio_input_file`.
- Display options/config: `display_mode`, `zoom`, `maxFramesPerSecond`,
  `showFPS`, `DisplayDevice::text_on_term`, `text_size`, `fontSize`,
  `disp_size`, `bypp`, `bytes_per_line`, `draw_mode`, `screenSizes`,
  `bufferSizes`, X11 flags in `xcthugha.h`, and `xcth_font`.
- Auto-change/message options: `changeQuiet`, `changeWaitMin`,
  `changeWaitRandom`, `changeCumulativeFireLevel`, `lock`, `change_little`,
  `changeMsgTime`, `paletteSmoothingChance`, `sound_minnoise`.
- Visual effect selections: `screen`, `flame`, `flameGeneral`, `wave`,
  `waveScale`, `table`, `object`, `translation`, `palette`, `border`,
  `flashlight`, `audioProcessing`, `use_translates`, `use_objects`.
- File/path config: `extra_lib_path`, `ini_file_override`, `display_prt_file`,
  `Keymap::keymapFile`.

### Registries And Catalogs

- `EffectControl::first` is the central hidden registry used by initial option
  resolution, random changes, save/restore, ini persistence, and presets.
- `effectPresetCatalog` is process-wide and walks the hidden effect registry.
- `Action::head`, `Keymap::first`, `Keymap::current`, `Interface::head`, and
  `Interface::current` are hidden registries/current selections.
- Mutable catalog/list globals include `paletteEntries`, `screenEntries`,
  `_flames`, `_objects`, wave/object/table lists, and audio processor entries.
- Mostly immutable catalogs still have global exposure: `screenCatalog`,
  `flameCatalog`, `waveCatalog`, `keyAssoc`, generated/default keymap data.
- Math lookup tables `sine` and `sin360` are mutable process-wide tables filled
  by `init_imath()`.

### Runtime Object Aliases

- `CthughaBuffer::buffer` and `CthughaBuffer::current`.
- `cthughaDisplay`, `displayDevice`, `displayBackend`, `displayRuntime`.
- `autoChanger`.
- `videoDirector()` singleton.
- `sceneCommandsForLegacyCallbacks()` global bridge.

### Audio State

- `AudioRuntime.cc` owns file-scope pointers, threads, atomics, chunks,
  completion flags, visual clock state, and current `AudioFrame`.
- `AudioFrame.cc` exposes global facade functions and silent fallback buffers.
- `audioAnalyzer`, `audioMetrics`, and `acousticContext` are global.
- `AudioVisualBridge` reads global audio processing/analyzer/facade state and
  creates `autoChanger` through a global pointer.
- `AudioProcessor.cc` has global FFT tables and a global `AudioProcessor`.
- `Border.cc` still calls `audioFrameRawData()` directly even though it also
  receives `VideoFrameContext`.

### Video And Screen Rendering State

- `VideoDirector.cc` is a singleton and still reads `cthughaDisplay`,
  `CthughaBuffer::buffer`, `maxFramesPerSecond`, `changeMsgTime`, and
  `paletteSmoothingChance`.
- `SceneCommands` is explicit as a facade, but it still reads every visual
  option singleton to build `SceneSettings`.
- `display.cc` renderers are partly explicit through `ScreenRenderContext`, but
  `screen_hfield`/`prepare_3d` read `acousticContext`, `screen_bent` reads
  `audioMetrics`, and `screen_zick` reads `audioFrameProcessedWaveData()`.
- `display.cc` also keeps renderer state in file-scope statics:
  `perm_lines`, `height_offset`, `s1`, `s2`, `p`, `rot`, `scaleFactor`,
  `scaleFactorPhase`, `splatSize`, `zicks`, and function-local animation
  state. These should be per-renderer state, not ambient state.

### Display Backend And Overlay State

- X11 platform globals: `xcth_display`, `xcth_app_con`, `DisplayDeviceX11::xcth_toplevel`.
- Backend frame layout globals: `disp_size`, `bypp`, `bytes_per_line`,
  `draw_mode`, `colormapped`, `bitmap_colors0..3`.
- Text/overlay globals: `text_size`, `fontSize`, `DisplayDevice::text_on_term`,
  static text colors, `errors`, and temporary `ScopedOverlayDisplayDevice`
  swapping the global `displayDevice`.
- X11 panel callbacks are static and depend on global options/catalogs even
  when the panel has `SceneCommands`.

### Input And UI State

- Key input: `x11_key`, `key_esc`, `ncurses_use`, `keyAssoc`, `nKeyAssoc`.
- UI current editing state: `currentOption`, `currentEffectControl`,
  `currentOptionInterfaceElement`.
- Key actions call directly into globals: `screen`, `zoom`, `audioProcessing`,
  `displayDevice`, `cthugha_close`, and `sceneCommandsForLegacyCallbacks()`.

### Ini, Resources, And Platform State

- `long_options` is a global table with `getopt_long` flag pointers directly
  into option globals.
- `ini_file`, `ini_nr`, `ini_file_path`, `optindsave`, and X resource
  `database` are ambient parser state.
- `PlatformLifecycle.cc` uses process-level static signal state. The signal
  flag itself may remain internal to the platform service, but the application
  dependency on lifecycle requests should be explicit.
- `Mixer.cc` keeps global mixer setup state.
- `AudioInternal.cc` keeps process-wide audio dump state.

### Stale Or Suspicious Compatibility Surface

These are declared or defined but appear unused in the current production path:
`screen_first`, `cth_main`, `cthugha_mode_text`, `cur_palette`,
`display_bitmap`, `display_text_time`, and the global `rev_byte_order`
declaration in `display.h` while X11 uses a member named `rev_byte_order`.
Each should be deleted or covered by a build target that proves it is still
needed.

## Target Shape

Application should own one `ApplicationContext` made of explicit services:

- `ShutdownController`
- `Clock`
- `RandomSource`
- `Logger`
- `PathConfig`
- `AppOptions`
- `EffectRegistry`
- `VisualCatalogs`
- `AudioSystem`
- `AudioFrameProvider`
- `AudioAnalysisState`
- `VideoDirector`
- `CthughaBuffer`
- `InterfaceManager`
- `KeymapRegistry`
- `DisplayRuntimeOwnership`
- `IniStore`
- `PlatformLifecycle`

Subsystems should receive only the subset they need. For example,
`AutoChanger` should receive clock, random source, auto-change options,
analysis state, `SceneCommands`, and quiet-message policy. It should not read
global audio metrics, global time, global random, or `videoDirector()`.

## Refactor Plan

### Phase 0: Guard Rails

- Add grep/source-boundary tests that fail on new production uses of known
  globals outside their current owner files.
- Expand existing display boundary tests to include `display.cc` references to
  `audioMetrics`, `acousticContext`, and audio-frame facade functions.
- Record current manual smoke commands for X11, MIT-SHM, audio playback,
  screen modes, overlays, and ini persistence.
- Delete or prove the stale declarations listed above.

### Phase 1: Runtime Services

- Introduce `ShutdownController`, `Clock`, `RandomSource`, and `Logger`.
- Make `Application` own these services.
- Replace `cthugha_close` reads/writes with shutdown requests first at call
  sites already owned by `Application`, key actions, audio completion, X11 quit,
  and credits.
- Replace `getTime()`/`gettime()` consumers with injected clocks as each owning
  class is touched. Keep the old functions only as temporary adapters.
- Replace `rand()`/`Random()` with injected `RandomSource` in auto-change,
  messages, palette generation, effect randomization, waves, and random audio.

### Phase 2: Explicit Options And Registries

- Add `AppOptions` to own all scalar/string options and path config.
- Add `EffectRegistry` to own registered `EffectControl` objects explicitly.
- Change `EffectControl` construction to register with `EffectRegistry&`
  instead of `EffectControl::first`.
- Move save/restore, random change, ini usage, and preset iteration onto the
  registry.
- Replace `long_options` static flag pointers with an `OptionParser` that
  writes into `AppOptions` through explicit setters.
- Keep global option aliases temporarily only as forwarding references, then
  remove them once all consumers take `AppOptions&` or narrower option groups.

### Phase 3: Visual Catalogs And Scene Options

- Add `VisualCatalogs` for screen, flame, wave, object, table, translation,
  palette, border, flashlight, image, and audio-processing choices.
- Make `SceneCommands` depend on `VisualCatalogs&`, `EffectRegistry&`, and
  `CthughaBuffer&`; remove direct reads of the visual option globals.
- Move `effectPresetCatalog` into `Application` or `EffectRegistry`.
- Replace `sceneCommandsForLegacyCallbacks()` by passing `SceneCommands&` into
  key actions, UI elements, X11 panel callbacks, and option adapters.

### Phase 4: Buffer And Video Director Ownership

- Make `Application` own `CthughaBuffer` as a member.
- Pass `CthughaBuffer&` to option parsing, visual catalog initialization,
  `SceneCommands`, `VideoDirector`, image loading, translation generation, and
  display fallback paths.
- Make `VideoDirector` an `Application` member instead of a function-local
  singleton.
- Inject `FrameBudgetSource`, palette smoothing options, quiet-message options,
  random source, and buffer into `VideoDirector`.
- Remove `CthughaBuffer::current`; then remove `CthughaBuffer::buffer`.

### Phase 5: Audio Runtime And Analysis

- Convert `AudioRuntime.cc` file-scope state into an `AudioRuntime` class.
- Make `AudioSystem` own `AudioRuntime` and expose an `AudioFrameProvider`
  interface.
- Convert `AudioFrame` facade functions into a temporary adapter over the
  provider, then pass the provider explicitly to `Application`, `AudioProcessor`,
  `AudioAnalyzer`, `BorderFilter`, and screen render context creation.
- Make `AudioAnalyzer` own `AudioMetrics` and `AcousticContext`; pass an
  `AudioAnalysisState&` to `AudioVisualBridge`, `AutoChanger`, `VideoFrameContext`,
  and `ScreenRenderContext`.
- Make `AudioVisualBridge` own `AutoChanger` as a member, not through the global
  `autoChanger` pointer.
- Update `Border.cc` and `display.cc` so all audio reads come from frame/context
  objects.

### Phase 6: Display Runtime And Backend State

- Create `DisplayConfig` for display mode, X11 flags, font, panel/root/private
  colormap choices, and initial window/buffer size choices.
- Move `disp_size`, `bypp`, `bytes_per_line`, `draw_mode`, `text_size`,
  `fontSize`, and text-on-terminal state into `DisplayDevice`/`DisplayBackendX11`
  owned state.
- Move `bitmap_colors0..3` and `colormapped` into `DisplayBackendX11`; pass the
  active native-pixel lookup table to `PixelTransfer::indexedToNative`.
- Replace `displayDevice`, `displayBackend`, `displayRuntime`, and
  `cthughaDisplay` aliases with explicit references from `Application`,
  `CthughaDisplay`, overlay producers, key actions, screenshot, and panels.
- Replace `ScopedOverlayDisplayDevice` with an overlay sink that directly uses
  the target display instance.

### Phase 7: UI, Input, And Key Actions

- Add `InputQueue` owned by `Application` or display runtime. X11 and ncurses
  push translated keys into it instead of using `x11_key`/`getkey()`.
- Add `InterfaceManager` to own interfaces and current selection.
- Add `KeymapRegistry` to own keymaps and action bindings.
- Replace `ACTION` static registration with explicit action registration that
  binds a `CommandContext` containing shutdown, scene commands, display
  commands, audio commands, interface manager, and ini writer.
- Remove `currentOption`, `currentEffectControl`, and
  `currentOptionInterfaceElement` by passing selected option context into action
  execution.

### Phase 8: Ini, Files, And Platform Adapters

- Replace `ini_file`/`ini_nr`/`ini_file_path` with `IniReader` and `IniWriter`
  objects.
- Make X resources an `IniSource` implementation that owns the X resource
  database explicitly.
- Move screenshot path, library paths, audio device paths, pulse settings, and
  mixer settings into config objects owned by `AppOptions`.
- Keep signal-handler storage internal to `PlatformLifecycle`, but expose only
  explicit lifecycle requests to `Application`.
- Move audio output dump state into an `AudioDumpWriter` owned by audio output or
  audio runtime.

### Phase 9: Renderer-Local State

- Convert stateful screen renderers into renderer objects or state records owned
  by `PresentationComposer`.
- Move `display.cc` 3-D/zick/perm state out of file-scope statics.
- Give `ScreenEntry` either a stateless render function plus state factory, or a
  renderer object interface.
- Move `AudioProcessor` FFT tables into `AudioProcessor` instance state or
  immutable precomputed tables owned by an audio processing service.
- Move `sine`/`sin360` into a `MathTables` service if they remain mutable
  initialization data.

### Phase 10: Remove Compatibility Aliases

- Remove `extern` declarations for mutable runtime state.
- Keep immutable catalogs either file-local or owned by explicit catalog
  objects.
- Add a final source test that production code has no direct references to:
  `cthugha_close`, `cthughaDisplay`, `displayDevice`, `displayBackend`,
  `displayRuntime`, `CthughaBuffer::buffer`, `CthughaBuffer::current`,
  `videoDirector()`, `autoChanger`, `EffectControl::firstRegistered()`,
  `Interface::current`, `Keymap::action(...)` static dispatch, global audio
  facade functions, global audio metrics, global display geometry, and mutable
  option globals.

## Good First Slices

1. Extend `ScreenRenderContext` with optional audio metrics, acoustic context,
   and processed audio pointers; remove direct audio globals from `display.cc`.
2. Change `apply_border()` to use `VideoFrameContext::rawAudioData` with a
   silent fallback instead of calling `audioFrameRawData()`.
3. Make `AudioVisualBridge` own `AutoChanger` as a member and expose status
   through the bridge or `Application`.
4. Replace `VideoDirector& videoDirector()` with an `Application` member and
   pass references to the few consumers that currently call the singleton.
5. Introduce `EffectRegistry` while keeping existing global options, then move
   `EffectControl::changeAll/changeOne/save/restore` and preset iteration onto
   the registry.

Each slice should preserve the existing tests:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
git diff --check
```
