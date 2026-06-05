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
Startup configuration acquisition is no longer one of those ambient areas:
`ConfigurationBuilder` builds the startup `Config`, and `Application` passes
immutable slices to subsystem startup APIs.

## Progress Audit Since 73fd509

This status was cross-checked against commits `744bd98..d280f83` on
`modulerefactor` and a source scan of current `src/`.

- Done: startup Configuration is explicit. Typed `Config` slices,
  `ConfigurationBuilder`, source strategies/patches/diagnostics, startup
  acquisition, and most legacy startup parser globals have been replaced.
- Done: Runtime Reconfiguration is explicit enough for the current module
  boundary. `RuntimeCommand`, `RuntimeCommandSink`, `RuntimeChangeMediator`,
  `RuntimeDisplayControls`, `RuntimeAudioControls`,
  `RuntimeAutoChangeControls`, `RuntimeEffectControls`,
  `RuntimePersistence`, `RuntimeShutdown`, and `RuntimeConfigRegistry` route
  live changes without the mediator mutating subsystem globals directly.
- Done: Audio has been substantially deglobalised. `AudioRuntime` and audio
  frame facade functions are gone; `AudioIngest`, `DecodedAudioHistory`,
  `AudioPassthrough`, owned `AudioFrame`, owned `AudioProcessor`/FFT state,
  owned audio processing selection, owned output config/dump state, owned OSS
  mixer session/controls, and instance-local audio diagnostics are in place.
- Done: Interface runtime state that belonged to Commands/Input has moved into
  Application-owned `InterfaceRuntime`: current interface selection, interface
  registration, runtime config/status adapters, save/status flags, and scoped
  option/effect command context.
- Partial: Application Lifecycle now owns shutdown request state through
  `RuntimeCloseState`/`RuntimeShutdown`, and `Application` owns the major
  runtime roots. Process services are not yet fully explicit: `gettime()`,
  `getTime()`, `now`, `deltaT`, `FramePacer`, `rand()`/`Random()`, logging
  verbosity, and the stale `cthugha_close` declaration/definition remain.
- Partial: Commands And Input is only halfway through. `InterfaceRuntime`
  removes the old interface current/edit globals, but `Action::head`,
  `Keymap::first`, `Keymap::current`, static `Keymap::action(...)`,
  `x11_key`, and `key_esc` remain.
- Partial: Scene owns more state through `Scene`/`SceneCommands`, owned
  auto-change settings, and runtime controls, but `EffectControl::first`,
  visual catalog/list globals, `videoDirector()`, and the scene command
  compatibility bridge remain. `AutoChanger` is no longer global, but it still
  lives in `AudioVisualBridge` rather than as Scene-owned policy.
- Partial: Frame Mutation and Display still have the older display/frame
  globals and renderer-local statics. Audio is now passed through frame
  contexts, but `CthughaBuffer::buffer`, display backend globals, overlay
  globals, and renderer state still need their own passes.

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
- Logging: `cthugha_logging_verbosity` plus `CTH_*` macros; effectively a
  global logger adapter configured from `LoggingConfig`.
- Shell execution: `systemf()` still exists as a process helper, though the
  compressed-asset loader path that needed it has been removed.

### Option And Configuration Globals

- Core startup choices now live in `AppConfig` and `LoggingConfig`; the
  remaining options below are runtime/editable state or compatibility mirrors
  populated from config slices.
- Done since 73fd509: Audio startup/session configuration is no longer held in
  option globals. `AudioConfig`, `AudioSettings`, `PcmFormat`,
  `AudioOutputConfig`, and `AudioOutputDump` are passed through factories and
  owned runtimes. The old names such as `soundSampleRate`, `soundChannels`,
  `pulse_server`, and `audio_output_dump` may survive as config keys or owned
  fields, not process-wide mutable options.
- Display options/config: `zoom`, `maxFramesPerSecond`, `showFPS`,
  `text_size`, `fontSize`, `disp_size`, `bypp`, `bytes_per_line`, `draw_mode`,
  `screenSizes`, `bufferSizes`, X11 flags in `xcthugha.h`, and `xcth_font`.
- Done since 73fd509: Auto-change runtime values moved into owned
  `AutoChangeSettings`/`AutoChangeControls`; `sound_minnoise` moved into
  audio-analysis startup config consumed by `AudioVisualBridge`.
- Still remaining here: display options, message/palette smoothing values, and
  visual effect selection state.
- Visual effect selections: `screen`, `flame`, `flameGeneral`, `wave`,
  `waveScale`, `table`, `object`, `translation`, `palette`, `border`,
  `flashlight`, `use_translates`, `use_objects`.
- Done since 73fd509: `audioProcessing` is no longer a global option; it is
  owned by `AudioProcessingState`/`AudioProcessingSelector`.
- File/path startup config now lives in `PathConfig`/`InputConfig` slices.

### Registries And Catalogs

- `EffectControl::first` is the central hidden registry used by initial option
  resolution, random changes, save/restore, ini persistence, and presets.
- `effectPresetCatalog` is process-wide and walks the hidden effect registry.
- `Action::head`, `Keymap::first`, and `Keymap::current` are hidden
  registries/current selections.
- Done since 73fd509: `Interface::head`, `Interface::current`, and current
  option/effect edit globals moved into Application-owned `InterfaceRuntime`.
- Mutable catalog/list globals include `paletteEntries`, `screenEntries`,
  `_flames`, `_objects`, wave/object/table lists, and audio processor entries.
- Mostly immutable catalogs still have global exposure: `screenCatalog`,
  `flameCatalog`, `waveCatalog`, `keyAssoc`, generated/default keymap data.
- Math lookup tables `sine` and `sin360` are mutable process-wide tables filled
  by `init_imath()`.

### Runtime Object Aliases

- `CthughaBuffer::buffer` and `CthughaBuffer::current`.
- `cthughaDisplay`, `displayDevice`, `displayBackend`, `displayRuntime`.
- Done since 73fd509: the process-wide `autoChanger` pointer was removed.
  `AudioVisualBridge` owns the current `AutoChanger` instance temporarily.
- `videoDirector()` singleton.
- `sceneCommandsForLegacyCallbacks()` global bridge.

### Audio State

Done since 73fd509:

- `AudioRuntime.cc`/`.h` were deleted. `Application` owns `AudioIngest`, which
  owns acquisition, decoded history, visual pacing, and current `AudioFrame`.
- `AudioFrame` facade functions and silent fallback globals were removed.
- `audioAnalyzer`, `audioMetrics`, and global `acousticContext` were removed;
  `Application` owns `AcousticContext` and passes frame metrics through
  `VideoFrameContext`/`ScreenRenderContext`.
- `AudioProcessor` is an owned instance. FFT state is behind
  `AudioFftProcessor`, with the old lookup/static tables removed.
- Audio processing selection is owned by `AudioProcessingState` and changed
  through `AudioProcessingSelector`/`RuntimeAudioControls`.
- PCM history, passthrough, output config, output dumping, Pulse diagnostics,
  submitted-PCM diagnostics, and OSS mixer controls are instance-owned.

Remaining audio-adjacent work:

- `AudioVisualBridge` still combines audio processing/analysis with
  AutoChanger orchestration. Scene-owned auto-change policy should replace
  that bridge.
- Audio still receives visual frame-window sizing from `Application`; keep that
  explicit until Frame Mutation owns frame geometry.
- Audio still uses process clock/random/logging adapters in places; those move
  with Application Lifecycle/Process Services.

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
- Text/overlay globals: `text_size`, `fontSize`,
  static text colors, `errors`, and temporary `ScopedOverlayDisplayDevice`
  swapping the global `displayDevice`.
- X11 panel callbacks are static and depend on global options/catalogs even
  when the panel has `SceneCommands`.

### Input And UI State

- Key input: `x11_key`, `key_esc`, `keyAssoc`, `nKeyAssoc`.
- Done since 73fd509: UI current editing state moved into scoped
  `InterfaceRuntime` command context; key quit routes through runtime commands
  and `RuntimeShutdown`.
- Remaining: key actions/static keymaps still call into global registries or
  static dispatch paths (`screen`, `zoom`, `displayDevice`,
  `sceneCommandsForLegacyCallbacks()`, `Keymap::action(...)`), and raw X11 key
  input still uses globals.

### Ini Persistence And Platform State

- `IniFiles.cc` keeps process-wide writer state and a persisted startup
  `Config` snapshot for runtime save commands.
- `DisplayDeviceX11::cth_init()` still lets Xt consume toolkit command-line
  options/resources during X shell initialization.
- `PlatformLifecycle.cc` uses process-level static signal state. The signal
  flag itself may remain internal to the platform service, but the application
  dependency on lifecycle requests should be explicit.
- Done since 73fd509: OSS mixer state moved into `MixerSession`/`MixerDevice`
  and UI adapters; submitted PCM dumping moved into `AudioOutputDump`, and
  submitted-PCM debug throttling is per `AudioOutput`.

### Stale Or Suspicious Compatibility Surface

These are declared or defined but appear unused in the current production path:
`screen_first`, `cth_main`, `cthugha_mode_text`, `cur_palette`,
`display_bitmap`, `display_text_time`, and the global `rev_byte_order`
declaration in `display.h` while X11 uses a member named `rev_byte_order`.
Each should be deleted or covered by a build target that proves it is still
needed.

## Target Shape

Application should own one composition root, but the primary architecture should
be modules, not a bag of peer services. `ApplicationContext` is allowed to wire
module roots together; it must not become a service locator. Production code
should receive the narrow collaborator it needs through constructors or method
parameters. Only `Application` and module-root constructors should know about
the whole composition root.

The module boundaries are:

- Application Lifecycle
- Configuration
- Runtime Reconfiguration
- Commands And Input
- Scene
- Audio
- Frame Mutation
- Display

Subsystems should receive only the subset they need. For example,
the current `AutoChanger` behavior should become Scene-owned
`SceneChangeScheduler` policy that receives clock, random source,
auto-change options, audio analysis snapshots, and a scene command target. It
should not read global audio metrics, global time, global random, or
`videoDirector()`.

## Module Boundary Definitions

Services are candidate implementation details of modules. A service that fits
cleanly may be implemented under that module. A service that only probably fits
needs a boundary test before implementation. A service marked split-required
should not be implemented as a single object because doing so would preserve
implicit coupling under a new name.

### Application Lifecycle Module

- Boundary: Owns process startup, run-loop sequencing, shutdown, platform
  lifecycle hooks, and construction/destruction order for all other modules.
- Owns/replaces: shutdown requests, signal/lifecycle polling, process-level time
  and randomness sources where they are genuine infrastructure, and startup
  diagnostics.
- Inputs: command-line arguments, platform lifecycle notifications, module-root
  factories, and validated startup configuration.
- Outputs: explicit module roots, run-loop ticks, shutdown requests, and final
  teardown ordering.
- Allowed dependencies: configuration outputs during startup; module roots only
  through narrow lifecycle interfaces such as `start()`, `poll()`, `stop()`,
  and `shutdownRequested()`.
- Forbidden dependencies: visual catalogs, mutable scene state, display device
  globals, audio frame state, and whole-`ApplicationContext` access outside the
  composition root.
- Lifecycle: built first; installs platform lifecycle hooks before subsystem
  startup; sequences configuration, scene, display, audio, input, frame
  mutation, and display presentation; tears modules down in reverse order.
- Fits cleanly: `ShutdownController`, `PlatformLifecycle`.
- Probably belongs here: `Clock`, `RandomSource`, `Logger`. Each is
  cross-cutting infrastructure, so dependents must receive narrow interfaces
  and tests must prevent a service-locator style grab.
- Split-required: frame pacing belongs at the boundary between lifecycle and
  frame mutation/display, not inside a generic clock; overlay logging belongs
  to Display or Scene messages, not `Logger`.
- Does not fit: `ApplicationContext` itself. It is composition glue, not a
  service and not a dependency that ordinary production code may request.
- Missing candidate: an explicit `ExternalProcessRunner` or
  `ShellCommandRunner` for `systemf()` if legacy shell-out behavior remains.

### Configuration Module

- Boundary: Owns defaults, command-line parsing, ini loading,
  validation, path resolution, startup configuration schemas/descriptors, and
  persistence formats.
- Owns/replaces: scalar/string startup option globals, path globals, parser
  state, legacy option-table coupling, ini reader globals, and save/restore
  wiring for configurable state.
- Inputs: command-line arguments, supported environment sources, ini files,
  build defaults, and persistence sources.
- Outputs: immutable startup configuration, option schemas/descriptors,
  validated path resolution, and serialization adapters for modules that own
  configurable domain state.
- Allowed dependencies: process logging, filesystem/path adapters, and
  serialization adapters supplied by owning modules.
- Forbidden dependencies: direct audio/display/scene runtime objects, hidden
  mutation of module state during parsing, or persistence code walking module
  globals.
- Lifecycle: defaults are built before parsing; ini files, environment, and
  command line are applied during startup; Runtime Reconfiguration consumes the
  resulting schemas/descriptors after Configuration; final persistence runs
  after runtime modules stop exposing mutable state.
- Fits cleanly: none of the current candidate services fits perfectly as-is.
- Probably belongs here: `PathConfig` if it is narrowed into a path declaration
  and resolver; `AppOptions` if split into startup config, runtime options, and
  scene selection state; `IniStore` if split into config loading and
  persistence.
- Split-required: `AppOptions` historically combined startup-only config, live
  runtime controls, scene selection, display/audio settings, and UI-editable
  option metadata. `IniStore` should remain split between startup config
  loading, runtime persistence, and module-owned serializers.
- Does not fit: `EffectRegistry` persistence should not be owned by
  Configuration; Configuration should call a Scene-provided serializer instead.
- Missing candidates: `ConfigurationBuilder`, `ConfigSource`,
  `ConfigAcquisitionStrategy`, `ConfigPatch`, `ConfigSchema`,
  `DeferredLogBuffer`, `CommandLineParser`, `ConfigLoader`,
  `RuntimeOptionSchema`, `ConfigPersistence`, and `PathResolver`.

#### Configuration Module Findings

The Configuration module is now the first substantial module built by
`Application`. Its job is to acquire startup configuration needed by downstream
modules without constructing those modules and without mutating their state.

The implemented startup shape is:

```cpp
ConfigBuildResult startupConfig = buildStartupConfig(argc, argv);
Application handles diagnostics/help and then supplies slices:

cthugha_configure_logging(startupConfig.config.logging);
configureAudioOptions(startupConfig.config.audio);
configureCthughaDisplay(startupConfig.config.display);
configureAutoChanger(startupConfig.config.autoChange);
sceneCommands().applyStartupConfig(startupConfig.config.scene);
Keymap::init(startupConfig.config.input);
```

Internally, `buildStartupConfig()` performs a small bootstrap pass for
`--path`/`--ini-file`, then builds the final value through:

```cpp
ConfigurationBuilder builder(bootstrapDiagnostics);
builder.addDefaults()
    .addIniFile(...)
    .addEnvironmentVariables(...)
    .addCommandLine(argc, argv)
    .build();
```

The exact source list should be configurable and test-owned, but the precedence
rule should be fixed: every source produces a `ConfigPatch`; later patches
override earlier patches for the same key. Hardcoded defaults are the lowest
precedence. Command-line options are the highest precedence. A command-line
`--ini-file` override is source selection, not an ordinary runtime option: it
replaces the default ini-file source list with the requested file.

The completed replacement deliberately ends mutation during parsing.
Configuration building has exactly one product: a `Config` value, or a failed
`ConfigBuildResult` with diagnostics. It may preserve useful semantics such as
source precedence, aliases, validation rules, and defaults, but it must not
initialize subsystems, write runtime globals, stage callbacks, or perform
downstream work.

- `ConfigurationBuilder` owns source order and precedence. It does not know
  about audio, scene, display, or command objects.
- `ConfigAcquisitionStrategy` objects know how to read one source type:
  defaults, ini file, supported environment variables, and command line.
- Each strategy returns a `ConfigPatch` plus diagnostics. It must not mutate
  globals, load scene files, open audio devices, resize buffers, or touch
  display objects.
- `ConfigSchema` owns option names, aliases, types, validation, clamping, and
  deprecation/canonical-name warnings. This is the startup source of truth.
- `DeferredLogBuffer` records warnings/errors/debug traces produced while
  building config. After `LoggingConfig` is known, `Application` constructs the
  real logger and flushes buffered diagnostics according to the final level.
- Fatal parse errors should be returned as structured `ConfigDiagnostic`
  entries in a failed `ConfigBuildResult`, not printed directly from the parser.
- After config is built, `Application` decides initialization order and supplies
  only the required config slice to each module root. Audio receives
  `AudioConfig`; Display receives `DisplayConfig`; Scene receives `SceneConfig`
  plus `EffectPolicy`; Runtime Reconfiguration receives option descriptors and
  explicit module reconfiguration targets; Commands And Input receives
  `InputConfig` and command descriptors.

The produced `Config` should be a composite value, but consumers should receive
only their slice:

- `AppConfig`: application-level startup choices such as save-on-exit,
  continuation behavior, and process-level toggles.
- `LoggingConfig`: verbosity and diagnostic sink policy.
- `PathConfig` or `PathResolverConfig`: library paths, ini override, keymap
  path, audio file paths, and resource roots.
- `AudioConfig`: source mode, generation/file/live input settings, sample
  format/rate/channels, output/passthrough settings, mixer/device settings,
  dump path, and acoustic-analysis thresholds such as minimum noise.
- `DisplayConfig`: display mode, window size/position, X11 flags, font,
  colormap/shared-memory/fullscreen/panel policy, zoom, and FPS display.
- `SceneConfig`: initial screen/flame/wave/table/object/translation/palette/
  border/flashlight/audio-processing selections. After startup this becomes
  Scene-owned mutable state, not Configuration-owned state.
- `EffectPolicy`, `SceneTransitionPolicy`, and `MessagesConfig`: allowed effect
  catalogs/presets, transition behavior, and quiet-message/QOTD policy.
- `RuntimeOptionSchema`: descriptors for settings that can be edited at
  runtime and persisted. Configuration may define schemas and persistence keys,
  but Runtime Reconfiguration coordinates live edits and module owners keep the
  live values.
- `KeymapConfig`: selected keymap file and keymap-loading policy for Commands
  And Input.

Important pushback/constraints:

- `Config` should be read-only after `build()`. Mutable scene selection is not
  "the config changing"; it is Scene state changing from initial configuration
  and commands. Persisting that state should use Scene serializers.
- Runtime-editable options need an explicit owner. Process-level toggles belong
  to Application Lifecycle or Logging; audio tuning belongs to Audio; scene
  selections belong to Scene; display choices belong to Display. The UI should
  edit through Runtime Reconfiguration and typed command targets, not by
  writing `Config`.
- `ConfigurationBuilder` must not emit downstream commands. Current
  startup side effects such as loading quiet-message files, setting image
  loading, resizing `CthughaBuffer::buffer`, changing `audioProcessing`, or
  mutating mixer initial volumes are config values consumed by the owning
  module during startup.
- Source discovery may require a bootstrap pass, but that pass should also be
  explicit. `--path`, `--ini-file`, `--help`, and early logging/env controls
  can be parsed by a `BootstrapCommandLineSource` that returns
  `BootstrapConfig`, then the full builder runs with the selected sources.
- Xt toolkit options/resources remain outside Cthugha startup configuration.
  `Application` still passes a display argument vector to X11 shell
  initialization; if that becomes Cthugha config in the future, it should be
  captured as an explicit X11 slice.

Test targets for Configuration:

- Builder precedence: defaults < library ini < home/auto ini < local/extra ini
  < continuation < environment < command line.
- `--ini-file` override uses only the requested ini file plus defaults and
  command-line/env sources.
- Deferred diagnostics are buffered before `LoggingConfig` and flushed at the
  final verbosity level.
- Aliases/deprecated names produce diagnostics but canonical names win.
- Invalid values fail or clamp according to schema rules with source/line
  attribution.
- Consumers can be constructed with only `AudioConfig`, `DisplayConfig`,
  `SceneConfig`, etc.; tests should fail if a consumer needs the whole
  `Config`.
- No config source or parser writes to option globals, display globals, audio
  globals, scene globals, or keymap globals.

Current startup configuration status:

- Done: `ConfigDiagnostic`, `DeferredLogBuffer`, `ConfigPatch`,
  `ConfigSchema`, typed `Config` slices, source strategy interfaces, and tests
  for defaults, ini text, environment, command-line, and dependency boundaries.
- Done: `Application` builds typed startup `Config`, acts on diagnostics/help,
  and passes immutable slices to logging, input, audio, display/X11,
  autochanger, audio analysis, effect policy, scene transition, messages,
  scene, keymap, catalog loading, and ini persistence.
- Done: old parser files and globals for startup config acquisition were
  deleted or replaced with typed config/persistence entry points.
- Done since 73fd509: runtime persistence now reads explicit runtime
  contributors and owned settings instead of a global startup config snapshot.
- Remaining: Configuration still defines some schemas/keys for runtime-owned
  domains, and Scene/Display still need module-owned serializers before
  Configuration can be considered fully isolated from live runtime domains.

### Runtime Reconfiguration Module

- Boundary: Owns runtime setting changes after startup. It turns UI edits,
  key actions, command-line-style runtime commands, panel changes, automatic
  changes, and scripted changes into typed reconfiguration requests applied to
  the module that owns the live state.
- Why this follows Configuration: the startup `Config` refactor makes initial
  dependencies explicit, but it does not by itself stop `Option` from owning
  live values, parsing, validation, UI text, side effects, and persistence.
  This module must be addressed immediately after Configuration so the refactor
  does not merely replace startup globals while leaving runtime mutation hidden.
- Owns/replaces: the live-value responsibilities currently concentrated in
  `Option`, `OptionInt`, `OptionOnOff`, `OptionTime`, `EffectControl` as an
  `Option`, and UI option editing globals.
- Inputs: option schemas/descriptors from Configuration, command intents from
  Commands And Input, current snapshots from owning modules, and explicit
  module reconfiguration targets.
- Outputs: typed `RuntimeChangeRequest` values, validation diagnostics,
  applied-change results, updated module snapshots, and persistence snapshots.
- Initial scope to cover: audio source and audio tuning changes; scene/effect
  selections such as screen, flame, wave, object, translation, palette, border,
  flashlight, and audio-processing choices; display screen selection, zoom,
  FPS display, and backend presentation toggles.
- Ownership rule: Runtime Reconfiguration coordinates live edits, but it does
  not own all live state. Audio owns audio runtime state, Scene owns effect and
  scene selection state, Display owns display presentation state, and
  Application owns lifecycle-only toggles. The reconfiguration layer calls
  explicit module APIs instead of writing globals or mutating `Config`.
- Lifecycle: constructed after Configuration has produced startup config and
  option descriptors; attached to module roots during Application startup;
  applies changes only at safe boundaries, normally between frames or at
  explicit restart/reopen points; provides snapshots to persistence during
  shutdown.
Current runtime reconfiguration status:

- Done since 73fd509: `RuntimeCommand` and `RuntimeCommandSink` define the
  command boundary for key/UI/panel/auto-change requests.
- Done since 73fd509: `RuntimeChangeMediator` no longer mediates by mutating
  the old globals directly. It delegates to explicit control ports:
  display, audio, auto-change, effects, persistence, and shutdown.
- Done since 73fd509: persistence and shutdown were extracted from the
  mediator into `RuntimePersistence`/`IniRuntimePersistence` and
  `RuntimeShutdown`/`RuntimeCloseState`.
- Done since 73fd509: `RuntimeConfigRegistry` and runtime config
  contributors provide current-selection snapshots for UI display and
  persistence.
- Done since 73fd509: live audio-processing and auto-change edits now mutate
  owned state through runtime controls; OSS mixer edits route through
  `RuntimeAudioControls`/`MixerControls`.
- Remaining: `Option`/`EffectControl` still carry UI text/parsing and many
  scene/display live values. Scene and Display need their own live-state
  owners before `Option` becomes metadata/adapters rather than domain state.

### Commands And Input Module

- Boundary: Owns input event normalization, keymap lookup, command intent
  creation, command dispatch, and option-editing interaction state.
- Owns/replaces: `x11_key`, `key_esc`, `Action::head`,
  `Keymap::first`, `Keymap::current`, static keymap dispatch, key association
  globals, `Interface::head`, `Interface::current`, `currentOption`,
  `currentEffectControl`, and `currentOptionInterfaceElement`.
- Inputs: raw input events from Display/platform, keymap configuration,
  available command definitions, and editable option descriptors from
  Configuration and Scene.
- Outputs: typed command intents and explicit command execution through a
  narrow `CommandContext`.
- Allowed dependencies: `InputQueue`, `KeymapRegistry`, command registry,
  option editor model, and narrow command targets supplied per dispatch.
- Forbidden dependencies: direct pointers to scene globals, display globals,
  audio globals, shutdown globals, or the whole application context.
- Lifecycle: default command/key bindings are registered during startup; keymap
  files are loaded after paths are resolved; events are translated after each
  display/platform event pump; commands execute before frame mutation for that
  tick; UI edit state is saved during configuration persistence.
- Fits cleanly: none of the current candidate services fits cleanly without a
  split because both UI and keymap code currently combine registry and
  execution responsibilities.
- Probably belongs here: `KeymapRegistry` if reduced to input-to-command
  mapping; `InterfaceManager` if reduced to option-editing state and selection.
- Split-required: `KeymapRegistry::dispatch(...)` should become a separate
  `CommandDispatcher`; `InterfaceManager::renderOverlay(...)` crosses into
  Display; action definitions belong in command registration, not keymap data.
- Does not fit: scene changes, display commands, audio commands, and shutdown
  requests. Those are command targets supplied through `CommandContext`, not
  state owned by Commands And Input.
- Missing candidates: `InputQueue`, `CommandRegistry`, `CommandDispatcher`, and
  `CommandContext`.

Current Commands/Input status:

- Done since 73fd509: `InterfaceRuntime` is Application-owned and now holds the
  current interface, interface registry, runtime adapters, save/status flags,
  and scoped option/effect command context.
- Done since 73fd509: `Interface::head`, `Interface::current`,
  `currentOption`, `currentEffectControl`, and
  `currentOptionInterfaceElement` are gone from production code.
- Remaining: `Action::head`, `Keymap::first`, `Keymap::current`, static
  `Keymap::action(...)`, `Keymap::runtimeCommandSink()`,
  `Keymap::audioProcessingState()`, `Keymap::interfaceRuntime()`, `x11_key`,
  `key_esc`, and global key associations still need an explicit input/command
  registry and dispatch context.

### Scene Module

- Boundary: Owns scene configuration, effect/control registration, visual
  catalogs, current scene selection, preset state, scene-change policy, and
  message policy that is semantic rather than display-specific.
- Owns/replaces: visual effect selection globals, effect-control registry
  globals, preset catalog globals, visual catalog/list globals, scene command
  bridges, and the policy portion of automatic scene changes.
- Inputs: validated configuration, command intents, random source, clock/timing
  source, and audio analysis snapshots.
- Outputs: immutable or snapshotted `SceneSettings`, scene-change requests,
  effect/control updates, and render-ready scene descriptions for Frame
  Mutation.
- Allowed dependencies: Configuration option views, process clock/random
  interfaces, Audio analysis snapshots, and command targets.
- Forbidden dependencies: direct buffer mutation, display presentation, audio
  runtime frame storage, or display backend geometry globals.
- Lifecycle: catalogs/effects register after configuration defaults exist;
  startup configuration selects the initial scene; commands and auto-change
  policy may update scene state between frames; a scene snapshot is handed to
  Frame Mutation before rendering.
- Fits cleanly: `EffectRegistry`, `VisualCatalogs`.
- Probably belongs here: `AutoChanger` if renamed/reframed as
  `SceneChangeScheduler`; `SceneCommands` if it becomes a narrow command target
  over scene state rather than a compatibility bridge.
- Split-required: `VideoDirector` currently spans scene control, frame
  composition, display presentation, message overlays, timing, and palette
  smoothing. Only scene state and scene transitions belong here. Visual
  selections currently inside `AppOptions` should move into explicit scene
  state or a scene configuration view.
- Does not fit: `AudioVisualBridge`; audio should publish analysis snapshots and
  Scene should consume them through policy.
- Missing candidates: `SceneController`, `SceneState`, `SceneSnapshot`,
  `SceneChangeScheduler`, and `SceneSerializer`.

### Audio Module

- Boundary: Owns audio input/output runtime, audio frame acquisition, audio
  processing, analysis, audio device/mixer state, and audio dump writing.
- Owns/replaces: `AudioRuntime.cc` file-scope state, audio frame facade
  functions, silent fallback frame globals, audio analyzer/metrics/acoustic
  globals, audio processor globals, mixer globals, and audio dump globals.
- Inputs: startup audio configuration, resolved audio paths/devices, clock for
  synchronization, and lifecycle/shutdown requests.
- Outputs: `AudioFrame` snapshots, processed audio products, and
  `AudioAnalysisSnapshot` values for Scene and Frame Mutation.
- Allowed dependencies: audio options, path resolver, clock, logger, shutdown
  requester, and platform audio/file adapters.
- Forbidden dependencies: scene commands, display presentation, render buffers,
  or auto-change policy.
- Lifecycle: configured after Configuration; started before first visual frame;
  polled or advanced once per application tick; analysis snapshot is produced
  before scene policy and frame mutation; stopped before display teardown.
- Fits cleanly: low-level `PcmSource` implementations, `AudioBuffer`, and
  `AudioFrameBuilder` as Audio-owned implementation details.
- Probably belongs here: raw-only `AudioFrameProvider`,
  `AudioProcessingPipeline`, `AcousticContextTracker`, `AudioDumpWriter`,
  mixer setup, and silent frame provider.
- Split-required: `AudioFrameProvider` should expose raw/current frames; the
  processed wave belongs to an audio processing pipeline or frame product.
  `AudioAnalysisState::minimumNoiseSatisfied(...)` is Scene policy and should
  leave Audio. `AudioSystem` and `AudioRuntime` should split into acquisition,
  passthrough, processing, analysis, output dump, and mixer/device adapters.
- Does not fit: `AutoChanger` and `AudioVisualBridge`.
- Missing candidates: `AudioAcquisitionRuntime`, `AudioPassthrough`,
  `AudioProcessingPipeline`, `AudioAnalysisSnapshot`, `AudioFrameProducts`,
  `AudioDeviceConfig`, and `AudioDumpWriter`.

#### Audio Module Findings

The Audio module should be treated as a pipeline with five separate
responsibilities. They are related, but they should not collapse into one
runtime object:

1. Audio generation
   - Purpose: Produce PCM without depending on external live input. This covers
     random audio and decoded file-backed audio.
   - Current code: `RandomNoisePcmSource`, `WavPcmSource`, `Minimp3PcmSource`,
     `RawPcmSource`, `PcmSourceFactory`.
   - Output: interleaved PCM plus `PcmFormat`.
   - Dependencies to inject: audio source config, resolved file paths,
     `RandomSource&` for random generation, and `Logger&`.
   - Must not know: display geometry, visual effect state, auto-change policy,
     or output/playback devices.

2. Audio acquisition
   - Purpose: Turn one selected PCM source, whether generated/file/live, into
     the raw audio frame for the current visual tick.
   - Current code: `AudioRuntime.cc` buffered pipeline,
     `AudioInputProcessor`, `AudioBuffer`, and `AudioFrameBuilder`.
   - Output: `AudioFrame` or future `RawAudioFrameSnapshot` containing signed
     8-bit stereo samples, sample count, center sample, format metadata, and
     silence/completion status.
   - Dependencies to inject: selected `PcmSource`, clock or sample-position
     source, audio format/window config, logger, and shutdown requester for
     fatal/completion handoff.
   - Must not know: selected visual processor, acoustic policy, scene commands,
     display presentation, or global frame facades.

3. Audio passthrough
   - Purpose: Optionally play acquired/generated PCM through an output backend.
   - Current code: `AudioOutput`, `AudioNullOutput`, `AudioPulseOutput`,
     `AudioDSPOutput`, output service logic in `AudioRuntime.cc`, and audio
     dump helpers in `AudioInternal.cc`.
   - Output: submitted PCM and completion/drain status.
   - Dependencies to inject: output config, output backend factory, clock for
     diagnostics/pacing, logger, optional `AudioDumpWriter`, and the PCM buffer
     it drains.
   - Must not know: visual frame processing, acoustic analysis, or scene
     changes.
   - Note: generated/file PCM can be passed through; live-input passthrough
     should be an explicit loopback mode, not a side effect of acquisition.

4. Audio processing for visuals
   - Purpose: Convert one raw per-frame audio snapshot into the data the visual
     domain needs for rendering.
   - Current code: `AudioProcessor`, `AudioProcessingOption`,
     `audioProcessing`, global FFT tables, and writes to
     `AudioFrame::processedWaveData`.
   - Output: future `VisualAudioFrame` or `ProcessedAudioFrame` containing the
     processed wave data used by waves, zick, borders, and other renderers.
   - Dependencies to inject: processing mode/config, owned FFT tables or
     immutable lookup tables, and the raw audio frame.
   - Must not know: acquisition runtime, output passthrough, global option
     registry, display backend, or scene auto-change policy.

5. Acoustic context
   - Purpose: Maintain a stateful view of what the music is doing over time.
     This is slower semantic audio state, not renderer sample data.
   - Current code: `AudioAnalyzer`, `AudioMetrics`, `AcousticContext`,
     `sound_minnoise`, `audioAnalyzer`, `audioMetrics`, and
     `acousticContext`.
   - Output: future `AudioAnalysisSnapshot` containing frame metrics,
     smoothed intensity, attack/fire, cumulative fire, and noisy/quiet status.
   - Dependencies to inject: raw audio frame, acoustic-analysis config such as
     minimum noise threshold, and logger for bounded diagnostics.
   - Must not know: `AutoChanger`, scene-change thresholds, visual filterchain
     refresh, or display rendering.

The intended per-frame data flow is:

```text
PcmSource
  -> AudioAcquisitionRuntime
  -> RawAudioFrameSnapshot
      -> AudioProcessingPipeline -> ProcessedAudioFrame
      -> AcousticContextTracker  -> AudioAnalysisSnapshot

AudioAcquisitionRuntime -> optional AudioPassthrough -> AudioOutput/AudioDump

Scene receives AudioAnalysisSnapshot.
Frame Mutation receives RawAudioFrameSnapshot, ProcessedAudioFrame, and
AudioAnalysisSnapshot through its frame context.
Display receives pixels only.
```

Current smells confirmed in code:

- `AudioRuntime.cc` owns source acquisition, output passthrough, buffering,
  frame reconstruction, threading, visual clock alignment, playback completion,
  and shutdown requests in one file-scope singleton.
- `AudioFrame.cc` is a global facade over the current raw frame, processed
  frame, silent fallback, runtime tick, and processor change notification.
- `AudioProcessor.cc` mixes visual processing algorithms, option/catalog
  registration, global FFT tables, random option selection, and direct access
  to the global audio-frame facade.
- `AudioAnalyzer.cc` mixes pure analysis, stateful acoustic context,
  `sound_minnoise` option ownership, and global publication of metrics/context.
- `AudioVisualBridge` is not a good module boundary. It currently processes
  audio, analyzes audio, constructs/deletes `AutoChanger`, and calls scene
  policy. The replacement should be separate `AudioFramePipeline` plus
  Scene-owned `SceneChangeScheduler`.
- `visualMaxDimension` leaking into audio startup is a real cross-boundary
  dependency. It should become an explicit `AudioFrameWindowConfig` supplied by
  the composition root from display/frame geometry, not an implicit display
  dependency inside audio.

Audio module objects should be split this way:

- `PcmSource`: existing interface, kept as the low-level PCM producer. It
  should be constructed through an injected source factory using explicit
  config and paths.
- `AudioAcquisitionRuntime`: owns selected source, buffering, sample-position
  alignment, frame building, live/file strategy differences, and completion
  state.
- `AudioPassthrough`: owns optional output draining, output backend state, and
  dump writer integration.
- `AudioFrameProvider`: raw-frame-only view over acquisition output. It should
  not expose processed wave data.
- `AudioProcessingPipeline`: pure-ish per-frame processor from raw frame plus
  processing mode to processed visual audio.
- `AcousticContextTracker`: stateful analyzer from raw frame plus acoustic
  config to `AudioAnalysisSnapshot`.
- `AudioModule`: module root owned by `Application`; it wires the above
  objects and exposes narrow interfaces, not global facades.

The first audio refactor should add tests around these seams before moving code:

- Service tests: deterministic `PcmSource` generation, raw frame building from
  a fixture PCM stream, passthrough drain/completion, each visual processing
  mode, acoustic metrics/intensity/fire/cumulative-fire behavior, and silent
  fallback behavior.
- Dependent tests: `Application` advances audio through an injected module;
  frame context creation receives explicit raw/processed/acoustic snapshots;
  `AutoChanger` or its replacement consumes only `AudioAnalysisSnapshot`;
  `display.cc`, `Border.cc`, and waves consume audio only through frame context.
- Boundary tests: forbid new production uses of `audioFrameCurrent()`,
  `audioFrameRawData()`, `audioFrameProcessedWaveData()`, `audioFrameTick()`,
  `audioFrameChange()`, `audioAnalyzer`, `audioMetrics`, `acousticContext`,
  `audioProcessing`, `sound_minnoise`, and `audioRuntime*` outside named
  compatibility adapters while the migration is active.

Current Audio status since 73fd509:

- Done: `AudioIngest` replaced the public `AudioRuntime` facade and owns
  acquisition, decoded history, visual pacing, current frame, completion, and
  suspend/resume lifecycle.
- Done: `DecodedAudioHistory` and `AudioOutputStream` split acquisition
  history from passthrough output cursors.
- Done: `AudioPassthrough` and output backends drain PCM without deciding the
  current visual audio frame.
- Done: `AudioFrame` now owns raw/processed per-frame data and per-frame
  metrics; global audio-frame facade functions are gone.
- Done: `AudioProcessor` absorbed per-frame analysis, delegates FFT through
  `AudioFftProcessor`, and no longer owns global FFT tables.
- Done: `AudioProcessingState`/`AudioProcessingSelector` own sound-processing
  mode; runtime audio controls mutate the selector.
- Done: `AcousticContext` is Application-owned and passed explicitly through
  frame contexts.
- Done: startup/device audio configuration and output configuration are
  explicit (`AudioConfig`, `AudioSettings`, `PcmFormat`,
  `AudioOutputConfig`, `AudioOutputDump`).
- Done: OSS mixer state and controls are owned by `MixerSession`,
  `MixerDevice`, and `MixerControls`.
- Done: submitted-PCM and Pulse underflow debug throttles are instance-local.
- Remaining: replace `AudioVisualBridge` with a cleaner audio pipeline plus
  Scene-owned auto-change policy; move process clock/random/logging access out
  through Application Lifecycle; keep driving display/frame consumers from
  explicit `VideoFrameContext` until Frame Mutation owns that boundary.

Historical removal order inside Audio and its status:

1. Done in current shape: introduce explicit data products:
   `RawAudioFrameSnapshot`,
   `ProcessedAudioFrame`, `AudioAnalysisSnapshot`, and an aggregate
   `AudioFrameProducts` for one visual tick. The names changed: the current
   product is `AudioFrame` plus `VideoFrameContext`/`ScreenRenderContext`
   analysis pointers rather than separate snapshot classes.
2. Done in current shape: extract `AudioProcessingPipeline` from
   `AudioProcessor.cc` so processing
   works on explicit raw/processed buffers without touching the frame facade or
   option globals. The name used in code is `AudioProcessingSelector` plus the
   owned `AudioProcessor` instance.
3. Done in current shape: extract `AcousticContextTracker` from
   `AudioAnalyzer.cc` so analysis owns no
   globals and receives minimum-noise config explicitly.
4. Remaining: replace `AudioVisualBridge` with an audio-only per-frame pipeline that
   returns `AudioFrameProducts`; move automatic scene-change policy into Scene.
5. Done in current shape: convert `AudioFrameProvider` to raw-frame-only and pass raw/processed/audio
   analysis products into `VideoFrameContext`.
6. Done: split `AudioRuntime.cc` into acquisition and passthrough owners, preserving
   the current file/generator and live-input strategies.
7. Done for production audio facades: remove audio facade functions and global audio state once all consumers take
   injected audio products or module interfaces.

### Frame Mutation Module

- Boundary: Owns mutation of indexed frame buffers and renderer-local state. It
  turns scene snapshots plus audio/frame inputs into pixels.
- Owns/replaces: `CthughaBuffer::buffer`, `CthughaBuffer::current`,
  `display.cc` renderer statics, screen renderer state, translation/image
  working buffers, and mutable math/render lookup tables where they are render
  implementation details.
- Inputs: `SceneSnapshot`, `AudioFrame` or `AudioAnalysisSnapshot`, display
  dimensions, frame timing, and renderer configuration.
- Outputs: `IndexedFrame` or frame view ready for Display presentation.
- Allowed dependencies: Scene snapshots, Audio snapshots, display dimensions,
  clock/frame delta, and renderer-local random source where needed.
- Forbidden dependencies: display backend/device state, key/action state,
  global audio facades, scene option globals, or persistence.
- Lifecycle: buffer storage is sized after display/config resolution; renderer
  state is initialized for the selected scene; each tick mutates the next frame;
  the completed frame is handed to Display; renderer state is reset on scene or
  size changes.
- Fits cleanly: `CthughaBuffer` as owned state, not really as a service.
- Probably belongs here: `PresentationComposer`, `ScreenRenderContext`,
  renderer state factories, `MathTables` if tables are render-owned.
- Split-required: `VideoDirector` composition belongs here, but scene
  transition policy and display presentation do not. `AudioProcessor` FFT state
  belongs in Audio unless it is purely a render lookup table.
- Does not fit: `SceneCommands`, `DisplayRuntimeOwnership`, `AudioSystem`, or
  `KeymapRegistry`.
- Missing candidates: `FrameStore`, `FrameComposer`, `RendererRegistry`,
  `RendererStateFactory`, and `FrameMutationContext`.

### Display Module

- Boundary: Owns platform/window backends, display devices, geometry, native
  pixel transfer, overlays as rendered output, and raw event collection.
- Owns/replaces: `cthughaDisplay`, `displayDevice`, `displayBackend`,
  `displayRuntime`, X11 globals, display geometry globals, native pixel tables,
  overlay text/error globals, and global overlay device swapping.
- Inputs: display configuration, frame views from Frame Mutation, overlay
  messages, and platform events.
- Outputs: presented frames, raw input events for Commands And Input, display
  geometry snapshots, and backend lifecycle status.
- Allowed dependencies: display options, path resolver for fonts, logger
  diagnostics, platform/X11 adapters, and an input event sink.
- Forbidden dependencies: scene catalogs, audio runtime, key command dispatch,
  option mutation, or direct shutdown globals.
- Lifecycle: configured after startup configuration; opened before audio/video
  frame flow starts; event-pumped once per tick before command dispatch;
  presents after Frame Mutation; closes after audio/frame flow stops.
- Fits cleanly: none of `DisplayRuntimeOwnership` fits cleanly as currently
  drawn because it combines too many display concerns.
- Probably belongs here: display facade/device/backend ownership, geometry,
  native pixel transfer, and overlay sink after splitting.
- Split-required: `DisplayRuntimeOwnership` should become `DisplaySystem`,
  `DisplayBackend`, `DisplayGeometry`, and `OverlaySink`. `Logger` must not
  point at display overlays; overlays should subscribe to messages or receive
  explicit message output.
- Does not fit: input command dispatch, scene message policy, or frame
  composition.
- Missing candidates: `DisplaySystem`, `DisplayBackend`, `DisplayGeometry`,
  `OverlayMessageQueue`, and `OverlaySink`.

### Service Placement Audit

- Fits cleanly: `ShutdownController` -> Application Lifecycle;
  `PlatformLifecycle` -> Application Lifecycle; `EffectRegistry` -> Scene;
  `VisualCatalogs` -> Scene; low-level `PcmSource` implementations,
  `AudioBuffer`, and `AudioFrameBuilder` -> Audio; `CthughaBuffer` -> Frame
  Mutation as state.
- Probably belongs: `Clock` -> Application Lifecycle; `RandomSource` ->
  Application Lifecycle; `Logger` -> Application Lifecycle/Diagnostics;
  `PathConfig` -> Configuration; `AutoChanger` -> Scene as
  `SceneChangeScheduler`; `SceneCommands` -> Scene command target; raw-only
  `AudioFrameProvider`, `AudioProcessingPipeline`, and
  `AcousticContextTracker` -> Audio; display backend/device/geometry pieces ->
  Display.
- Split across modules: `AppOptions`, `IniStore`, `VideoDirector`,
  `DisplayRuntimeOwnership`, `InterfaceManager`, `KeymapRegistry`,
  `AudioAnalysisState` as currently written, `AudioFrameProvider` as currently
  written, `AudioSystem`, and `AudioRuntime`.
- Does not fit as a service: `ApplicationContext`, `AudioVisualBridge`,
  `CthughaBuffer` if treated as a peer service rather than frame storage, and
  any logger-to-overlay coupling.
- Missing from the original service list: `InputQueue`, `CommandDispatcher`,
  `CommandContext`, `SceneController`, `FrameComposer`, `DisplaySystem`,
  `PathResolver`, `ConfigLoader`, and an explicit shell/external-process
  runner for `systemf()`.

## ApplicationContext Service Contracts

This section is a service-contract audit, not the final module design. Any
service marked split-required above should be decomposed before implementation;
any service marked "probably belongs" needs an explicit boundary test before it
is allowed to settle in that module.

Status note since 73fd509: some contracts below landed under narrower names.
`ShutdownController` landed as `RuntimeShutdown`/`RuntimeCloseState`;
`InterfaceManager` partially landed as `InterfaceRuntime`; `AudioSystem`
split into `AudioIngest`, `AudioPassthrough`, output/config/dump classes,
`AudioProcessingState`/`AudioProcessingSelector`, and mixer owners rather than
one service. Keep the contracts below as shape guidance, not proof that the
old names still need to be implemented.


### `ShutdownController`

- Purpose: Own the application shutdown request state and the reason for
  termination.
- Globals replaced: `cthugha_close` and direct increments from key actions,
  audio completion, X11 quit, credits, and resume failure paths.
- Used by: `Application::run()`, key/action dispatch, X11 event handling,
  audio-runtime completion, credits/help sequences, and platform lifecycle
  polling.
- Provided to customers: Constructed by `ApplicationContext`; passed as
  `ShutdownController&` to coordinators and as a narrower
  `ShutdownRequester&` to code that may only request shutdown.
- Lifecycle timing: Created before option parsing so command-line errors can
  request an orderly exit; read before each frame wait, after each event pump,
  after audio advancement, after display presentation, and during final teardown.
- References contained: No external service references; owns an atomic or plain
  request flag plus optional enum/string reason.
- API surface: `request(ShutdownReason)`, `requested() const`,
  `reason() const`, `clearForTest()`, and optionally
  `throwIfRequested()` for startup paths that cannot continue.

### `Clock`

- Purpose: Provide monotonic application time, frame delta calculation, and
  testable timers without direct wall-clock calls.
- Globals replaced: `gettime()`, `getTime()`, `now`, `deltaT`,
  `CthughaDisplay.cc` static `FrameClock`, `Application.cc` static
  `FramePacer`, and ad hoc `time(0)` seeding.
- Used by: `Application::run()`, frame pacing, `CthughaDisplay`, audio runtime,
  display tracing, message/error expiry, credits/help animation, auto-change
  policy, and tests that need deterministic time.
- Provided to customers: `ApplicationContext` owns a concrete
  `SystemClock`; dependents receive `Clock&` or a `FrameTimer&` facade.
- Lifecycle timing: Created at process startup before random seeding; sampled
  once at startup, before each frame is composed, after event/audio work, before
  frame pacing sleep, and during shutdown diagnostics.
- References contained: No application references; may contain a steady-clock
  epoch, last-frame timestamp, and injected sleeper for tests.
- API surface: `now()`, `secondsSinceStart()`, `beginFrame()`,
  `frameDelta() const`, `sleepUntilFrameDeadline(FrameRateLimit)`, and
  `makeTimer(Duration)`.

### `RandomSource`

- Purpose: Centralize all nondeterminism and make random-dependent behavior
  reproducible in tests.
- Globals replaced: `srand(time(0))`, `rand()`, and `Random()`.
- Used by: option randomization, `AutoChanger`, palette generation, wave and
  flame setup, message choice, random audio, and any effect that currently
  reaches for process random state.
- Provided to customers: `ApplicationContext` owns the generator; dependents
  receive `RandomSource&`, while pure functions receive generated values where
  practical.
- Lifecycle timing: Seeded after `Clock` and before option/catalog
  initialization; used during initial random option resolution, scene changes,
  every frame that needs random visual/audio data, and tests with fixed seeds.
- References contained: Optional `Clock&` only for production seed creation;
  otherwise owns generator state and seed metadata.
- API surface: `seed(uint64_t)`, `seedFromClock(Clock&)`, `nextUInt()`,
  `uniformInt(min, max)`, `uniformFloat(min, max)`, `chance(probability)`, and
  `shuffle(range)`.

### `Logger`

- Purpose: Own diagnostics, verbosity, and structured application messages.
- Globals replaced: the process-wide logging verbosity switch and direct
  `CTH_*` macro dependency on global state.
- Used by: application startup, option/ini loading, display backend, audio
  runtime, resource loading, effect registration, and tests that assert warning
  behavior.
- Provided to customers: `ApplicationContext` owns a `Logger`; dependents
  receive `Logger&` or `LogSink&`. Macros become thin adapters during migration.
- Lifecycle timing: Created first, before option parsing; verbosity is updated
  after command-line and ini parsing; used throughout startup, per-frame
  diagnostics, error overlays, and teardown.
- References contained: Output sink, verbosity level, optional overlay message
  sink reference once display exists.
- API surface: `setVerbosity(int)`, `verbosity() const`, `trace(category,msg)`,
  `info(msg)`, `warn(msg)`, `error(msg)`, and `withSink(LogSink&)`.

### `PathConfig`

- Purpose: Own resolved filesystem paths and path-search policy separate from
  the option parser.
- Globals replaced: `extra_lib_path`, `ini_file_override`,
  `Keymap::keymapFile`, audio input/output filenames, and scattered device/path
  strings currently living as globals.
- Used by: ini loading/saving, resource loaders, keymap loading,
  audio input/output setup, and tests that need temporary resource roots.
- Provided to customers: Built by option parsing into `ApplicationContext`;
  passed as `const PathConfig&` to loaders and mutable `PathConfig&` only to
  parsers.
- Lifecycle timing: Defaults exist before command-line parsing; resolved after
  command line and before ini/resource/keymap/audio setup; read during startup
  and on explicit save/export actions.
- References contained: No service references; owns normalized path strings,
  search roots, and resolved file policy.
- API surface: `libraryPaths() const`, `iniOverride() const`,
  `displayDumpPath() const`, `keymapPath() const`, `audioInputFile() const`,
  `audioOutputDump() const`, `resolveResource(name)`, and setters used only by
  parsers/builders.

### `AppOptions`

- Purpose: Own scalar and string configuration as explicit state, grouped by
  subsystem.
- Globals replaced: core options, audio options, display options,
  auto-change/message options, and feature toggles listed in the option
  inventory.
- Used by: command-line parser, ini store, display runtime setup, audio system
  setup, video director, auto changer, visual catalog defaults, key actions,
  and UI option editors.
- Provided to customers: `ApplicationContext` owns one `AppOptions`; most
  dependents receive narrower views such as `AudioOptions&`, `DisplayOptions&`,
  `AutoChangeOptions&`, or `const AppOptions&`.
- Lifecycle timing: Defaults are constructed before parsing; command line and
  ini mutate it during startup; runtime UI/key actions mutate allowed live
  options between frames; teardown reads save/persistence settings.
- References contained: Owns value groups and may contain `PathConfig` by value
  or reference; should not reference runtime services.
- API surface: typed accessors for `core()`, `audio()`, `display()`,
  `autoChange()`, `messages()`, `paths()`, `visualSelections()`,
  `setFromOption(name,value)`, `snapshot()`, and validation helpers.

### `EffectRegistry`

- Purpose: Own the registered effect controls and their lifecycle operations.
- Globals replaced: `EffectControl::first`, `effectPresetCatalog` traversal of
  hidden registration state, and effect save/restore/random iteration through
  process globals.
- Used by: `Application` startup, option parsing, ini persistence,
  `SceneCommands`, preset loading/saving, random change actions, and UI option
  editors.
- Provided to customers: `ApplicationContext` constructs the registry before
  effect/control definitions are registered; constructors or registration
  functions receive `EffectRegistry&`.
- Lifecycle timing: Created before visual catalog initialization; populated
  during startup registration; read and mutated during option parsing, scene
  changes, randomization, per-frame option updates, and ini save on teardown.
- References contained: Owns or references registered `EffectControl` objects;
  may reference `RandomSource&` only for operations that randomize.
- API surface: `registerControl(EffectControl&)`, `find(name)`, `all()`,
  `changeOne(name,delta)`, `changeAll(ChangeRequest)`, `randomize(RandomSource&)`,
  `save(IniWriter&)`, `restore(IniReader&)`, and `presets()`.

### `VisualCatalogs`

- Purpose: Own named visual/audio-effect catalogs and resolve selected scene
  entries without global lists.
- Globals replaced: `paletteEntries`, `screenEntries`, `_flames`, `_objects`,
  wave/object/table lists, audio processor entries, `screenCatalog`,
  `flameCatalog`, `waveCatalog`, and related exposed catalogs.
- Used by: `SceneCommands`, option parsing, UI option panels, key actions,
  preset loading, image/table/translation setup, and tests that build small
  catalog fixtures.
- Provided to customers: `ApplicationContext` owns a `VisualCatalogs`;
  consumers receive `const VisualCatalogs&` for lookup and a builder/mutable
  reference only during startup registration.
- Lifecycle timing: Constructed after `EffectRegistry` and `AppOptions`
  defaults; populated during startup; queried during parsing and every scene
  change; stable during frame rendering except for explicit runtime catalog
  reload features if kept.
- References contained: References to registered effect controls and immutable
  catalog entries; no direct display/audio runtime references.
- API surface: `screens()`, `flames()`, `waves()`, `palettes()`, `borders()`,
  `flashlights()`, `audioProcessors()`, `findScreen(name)`,
  `resolve(SceneSelection)`, `registerEntry(...)`, and validation methods.

### `AudioSystem`

- Purpose: Own audio device/file runtime, threading, lifecycle, and the current
  frame source.
- Globals replaced: `AudioRuntime.cc` file-scope pointers, threads, atomics,
  chunk buffers, completion flags, visual clock state, mixer setup state where
  runtime-owned, and audio dump state when it belongs to output.
- Used by: `Application` startup/run/teardown, `AudioVisualBridge`,
  `AudioProcessor`, `AudioAnalyzer`, display frame context creation, and tests
  with fake input.
- Provided to customers: `ApplicationContext` owns `AudioSystem`; dependents
  receive `AudioSystem&` for lifecycle or `AudioFrameProvider&` for frame reads.
- Lifecycle timing: Configured after `AppOptions` and `PathConfig`; started
  before the first display frame; advanced or polled once per application frame;
  stopped before display teardown and before final ini save.
- References contained: `const AudioOptions&`, `PathConfig&` or resolved paths,
  `Clock&`, `Logger&`, optional `ShutdownRequester&`, device/file backends, and
  owned `AudioFrameProvider` implementation.
- API surface: `configure(AudioOptions, PathConfig)`, `start()`, `poll()`,
  `stop()`, `finished() const`, `frameProvider()`, `metricsSource()` if needed,
  and `setDumpWriter(AudioDumpWriter)`.

### `AudioFrameProvider`

- Purpose: Provide explicit access to raw and processed audio frames without
  global facade functions.
- Globals replaced: `audioFrameRawData()`, `audioFrameProcessedWaveData()`,
  silent fallback buffers in `AudioFrame.cc`, and current-frame file-scope
  storage in `AudioRuntime.cc`.
- Used by: `AudioProcessor`, `AudioAnalyzer`, `VideoFrameContext`,
  `ScreenRenderContext`, `BorderFilter`, `display.cc` renderers, and tests.
- Provided to customers: Exposed by `AudioSystem::frameProvider()`; passed as
  `AudioFrameProvider&` or `const AudioFrameProvider&` to frame builders and
  processors.
- Lifecycle timing: Available after `AudioSystem::configure()`; returns silent
  frames before audio starts, latest frames during each application frame, and a
  final stable/silent frame during shutdown.
- References contained: May reference `AudioRuntime` current frame storage or
  own a small silent-frame fallback; should not reference visual systems.
- API surface: `currentFrame() const`, `rawSamples() const`,
  `processedWave() const`, `sampleRate() const`, `channels() const`,
  `isSilent() const`, and `silentFrame()`.

### `AudioAnalysisState`

- Purpose: Own derived audio analysis data used by visuals and automatic scene
  changes.
- Globals replaced: `audioAnalyzer`, `audioMetrics`, `acousticContext`, and
  analysis reads in `display.cc`, `AutoChanger`, and `AudioVisualBridge`.
  The global `autoChanger` pointer is removed by moving automatic scene-change
  policy into the Scene module as `SceneChangeScheduler`.
- Used by: `AudioAnalyzer`, `AudioVisualBridge`, `AutoChanger`,
  `VideoFrameContext`, `ScreenRenderContext`, `display.cc` 3-D/bent/zick
  renderers, and border/flashlight effects that depend on audio.
- Provided to customers: `ApplicationContext` owns `AudioAnalysisState`;
  analyzers receive mutable access, visual consumers receive const views or
  per-frame snapshots.
- Lifecycle timing: Created before audio starts; updated after each audio poll
  and before video composition; snapshotted into frame contexts before screen
  rendering; reset during audio restart and teardown.
- References contained: Owns `AudioMetrics`, `AcousticContext`, analysis
  history, and maybe a reference to `AudioFrameProvider` during update calls.
- API surface: `update(const AudioFrameProvider&)`, `metrics() const`,
  `acoustic() const`, `snapshot() const`, `reset()`, and
  no scene-policy methods. The current
  `minimumNoiseSatisfied(AutoChangeOptions)` idea belongs in Scene policy, not
  Audio analysis.

### `VideoDirector`

- Purpose: Coordinate scene state, frame composition policy, messages,
  transitions, and display presentation decisions.
- Globals replaced: `videoDirector()` singleton, reads of `cthughaDisplay`,
  `CthughaBuffer::buffer`, `maxFramesPerSecond`, `changeMsgTime`, and
  `paletteSmoothingChance`.
- Used by: `Application::run()`, `SceneCommands`, key actions,
  `AudioVisualBridge`, auto-change, message/overlay code, and tests for scene
  transitions.
- Provided to customers: Owned by `ApplicationContext` after display, buffer,
  options, catalogs, and analysis services are available; passed explicitly as
  `VideoDirector&` or narrower `ScenePresenter&`.
- Lifecycle timing: Constructed after `CthughaBuffer` and display runtime
  setup; initialized before first frame; called once per frame after audio
  analysis and input handling; shut down before display backend teardown.
- References contained: `CthughaBuffer&`, display/presentation sink,
  `const DisplayOptions&`, `const MessageOptions&`, `RandomSource&`, `Clock&`,
  optional `AudioAnalysisState&`, and renderer/composer owned state.
- API surface: `initialize(SceneSettings)`, `composeFrame(VideoFrameContext)`,
  `present()`, `setScene(SceneSettings)`, `postMessage(Message)`,
  `setPaletteSmoothing(...)`, `frameBudget() const`, and `shutdown()`.

### `CthughaBuffer`

- Purpose: Own the indexed pixel buffers and expose buffer access explicitly to
  renderers, loaders, and display handoff code.
- Globals replaced: `CthughaBuffer::buffer` and `CthughaBuffer::current`.
- Used by: `Application`, `SceneCommands`, `VideoDirector`,
  `PresentationComposer`, image/translation generation, display fallback paths,
  and tests that build deterministic frame buffers.
- Provided to customers: `ApplicationContext` owns the instance; consumers
  receive `CthughaBuffer&`, `IndexedFrame&`, or immutable frame views.
- Lifecycle timing: Constructed after display/buffer size options are resolved;
  resized during display-mode changes; written during each render pass; read
  during presentation; destroyed after display presentation stops.
- References contained: Owns frame storage, dimensions, active buffer index,
  and possibly palette/translation working buffers; no references to services.
- API surface: `resize(Size)`, `currentFrame()`, `nextFrame()`,
  `swapFrames()`, `clear(index)`, `dimensions() const`, and `indexedView()`.

### `InterfaceManager`

- Purpose: Own UI interfaces, current selection/edit state, and option editing
  flow.
- Globals replaced: `Interface::head`, `Interface::current`, `currentOption`,
  `currentEffectControl`, and `currentOptionInterfaceElement`.
- Used by: key/action dispatch, option panels, X11 controls, text overlays,
  ini save/edit actions, and tests that exercise option navigation.
- Provided to customers: `ApplicationContext` owns `InterfaceManager`;
  actions receive `InterfaceManager&` or a narrower `OptionEditor&`.
- Lifecycle timing: Created after `AppOptions`, `EffectRegistry`, and
  `VisualCatalogs`; populated during startup UI registration; updated after
  input events and before frame composition; flushed/saved during teardown if
  required.
- References contained: References to editable option views, effect registry,
  visual catalogs, display text sink, and optional logger.
- API surface: `registerInterface(InterfaceDefinition)`, `current() const`,
  `selectNext()`, `selectPrevious()`, `beginEdit(OptionId)`,
  `adjustCurrent(delta)`, `renderOverlay(TextSink&)`, and `saveState(IniWriter&)`.

### `KeymapRegistry`

- Purpose: Own keymaps, action bindings, and dispatch context explicitly.
- Globals replaced: `Action::head`, `Keymap::first`, `Keymap::current`,
  `Keymap::keymapFile`, `keyAssoc`, `nKeyAssoc`, and static
  `Keymap::action(...)` dispatch.
- Used by: command-line/ini keymap loading, X11 input handling,
  key-action execution, UI tests, and help/credits displays.
- Provided to customers: `ApplicationContext` owns the registry; input systems
  receive `KeymapRegistry&` for translation/dispatch and actions receive an
  explicit `CommandContext`.
- Lifecycle timing: Defaults registered before keymap files are read; command
  line/ini can select or override keymaps during startup; dispatch occurs after
  each event pump and before audio/video composition for that frame.
- References contained: Registered action definitions, keymap definitions,
  selected keymap, `PathConfig` for loading, and maybe `Logger&`; no hidden
  references to scene/display/audio globals.
- API surface: `registerAction(ActionDefinition)`,
  `registerKeymap(KeymapDefinition)`, `load(Path)`, `select(name)`,
  `translate(InputEvent)`, `dispatch(Key, CommandContext&)`, and
  `bindings() const`.

### `DisplayRuntimeOwnership`

- Purpose: Own the display facade, backend, device, overlay sink, and backend
  configuration state as explicit objects.
- Globals replaced: `cthughaDisplay`, `displayDevice`, `displayBackend`,
  `displayRuntime`, `xcth_display`, `xcth_app_con`,
  `DisplayDeviceX11::xcth_toplevel`, `disp_size`, `bypp`, `bytes_per_line`,
  `draw_mode`, `colormapped`, `bitmap_colors0..3`, `text_size`, `fontSize`,
  `errors`, and `ScopedOverlayDisplayDevice`.
- Used by: `Application`, `CthughaDisplay`, `VideoDirector`, X11 callbacks,
  overlay/message rendering, key/display commands, and tests with fake display
  sinks.
- Provided to customers: `ApplicationContext` owns `DisplayRuntimeOwnership`;
  consumers receive narrow references such as `DisplaySink&`,
  `OverlaySink&`, `DisplayBackend&`, or `DisplayGeometryProvider&`.
- Lifecycle timing: Configured after options and path/resource setup; opened
  before audio/video starts; pumped for input and presented once per frame;
  overlays are flushed during composition; closed after audio/video shutdown.
- References contained: `DisplayOptions&`, `PathConfig&`, `Logger&`,
  platform/X11 state, backend/device/display instances, geometry, native pixel
  lookup tables, and overlay/error text state.
- API surface: `open(DisplayOptions)`, `pumpEvents(InputQueue&)`,
  `display()`, `backend()`, `device()`, `geometry() const`,
  `present(const IndexedFrame&)`, `overlaySink()`, `resize(Size)`, and
  `close()`.

### `ConfigPersistence`

- Purpose: Own ini writing and persistence coordination separate from startup
  config acquisition.
- Globals replaced: `IniFiles.cc` writer state, the persisted startup `Config`
  snapshot, and direct runtime save commands that call a global no-arg writer.
- Used by: application save-on-exit, UI save commands, stop-and-continue state,
  and module serializers for runtime-owned state.
- Provided to customers: `ApplicationContext` owns `ConfigPersistence`;
  services receive `ConfigWriter&` or a narrow persistence command target.
- Lifecycle timing: Created after startup config is built; serves explicit save
  commands during runtime; writes final state during teardown when enabled.
- References contained: `PathConfig&`, `Logger&`, current output file state, and
  serialization adapters supplied by module owners.
- API surface: `saveStartupSnapshot(Config)`, `saveContinuation(SceneSnapshot)`,
  `writer()`, and `diagnostics()`.

### `PlatformLifecycle`

- Purpose: Own platform signals and lifecycle integration without leaking
  process statics into application logic.
- Globals replaced: public dependency on `PlatformLifecycle.cc` signal statics
  and direct platform shutdown checks outside the lifecycle service.
- Used by: `Application` startup/run/teardown, signal handlers, X11 lifecycle,
  audio interruption handling, and tests that simulate termination requests.
- Provided to customers: `ApplicationContext` owns `PlatformLifecycle`;
  `Application` calls it directly, while signal handlers write only to its
  internal platform-safe flag.
- Lifecycle timing: Installed before subsystems start; polled before and after
  event/audio/frame work; uninstalled after display/audio teardown; reset
  between tests.
- References contained: Internal signal-safe flags, previous handlers, optional
  `ShutdownRequester&`, and `Logger&` for non-signal diagnostics.
- API surface: `install(ShutdownRequester&)`, `poll()`,
  `shutdownRequested() const`, `restore()`, `resetForTest()`, and
  `lastSignal() const`.

## Outside-In Refactor Plan

Every module/component migration follows the same outside-in loop. The API is
agreed and tested first, the dependent behavior is pinned next, and only then
does the implementation move global state behind the module boundary.

1. Identify the service API, including the narrow read/write interfaces its
   customers should receive.
2. Write service tests that fail because the service contract does not exist
   yet. These are red 1 tests.
3. Write dependent tests that fail because current dependents still use globals
   or lack injection points. These are red 2 tests.
4. Implement the service and its fakes/adapters until the service tests pass.
   This is green 1.
5. Update the dependents to receive the service explicitly until the dependent
   tests pass. This is green 2.
6. Recheck source for remaining implicit dependencies or globals that belong to
   that service. Add a boundary test for every remaining compatibility alias.
7. If the recheck finds more hidden dependencies, revise the API and repeat
   from step 1 for that service.

### Cross-Cutting Guard Rails

- Add a source-boundary test harness before the first service migration. It
  should be able to assert that a token is allowed only in named owner files and
  compatibility adapters.
- Keep compatibility aliases short-lived. Each alias must have an owner file, a
  removal condition, and a source-boundary test that blocks new consumers.
- Preserve current smoke coverage after each module/component migration:
  `cmake --build build`, `ctest --test-dir build --output-on-failure`,
  `tests/headers/check-headers.sh`, and `git diff --check`.
- Record manual smoke commands for X11, MIT-SHM, audio playback, screen modes,
  overlays, keymaps, and ini persistence before deleting the old globals.
- Delete or prove the stale declarations listed above as soon as the first
  boundary test harness exists.

### Module Migration Order

This order is outside-in at the module boundary. Within each module, apply the
seven-step loop to every clean service or split-out component before moving on.
Do not implement split-required services as single classes.

1. Application Lifecycle - partially complete
   - API first: module-root construction, shutdown requests, lifecycle polling,
     clock/random/logging interfaces, and run-loop sequencing.
   - Red 1 tests: lifecycle services can be faked independently and do not need
     any scene/audio/display globals.
   - Red 2 tests: `Application::run()`, key quit, X11 quit, audio completion,
     frame timing, random seeding, and diagnostics use injected lifecycle
     collaborators.
   - Green targets: `ShutdownController`, `PlatformLifecycle`, narrow `Clock`,
     narrow `RandomSource`, policy-free `Logger`, and deletion or ownership of
     unused shell-execution helpers.
   - Done since 73fd509: shutdown requests are Application-owned through
     `RuntimeShutdown`/`RuntimeCloseState`; direct `cthugha_close` mutation is
     gone from normal runtime paths.
   - Remaining: make clock, frame timing, random source, logging, and stale
     process helpers explicit.
   - Recheck target: no reads/writes of `cthugha_close`, `gettime()`,
     `getTime()`, `rand()`, `srand()`, `Random()`, lifecycle statics, or
     logging verbosity outside owner/adapters.

2. Configuration - startup acquisition/provisioning complete, persistence split partly complete
   - API first: `ConfigurationBuilder`, `ConfigSource` strategies,
     `ConfigPatch`, `ConfigSchema`, `DeferredLogBuffer`, immutable `Config`
     slices, `RuntimeOptionSchema`, `PathResolver`, `ConfigPersistence`, and
     serializers supplied by modules.
   - Red 1 tests: command-line, ini, environment, path resolution, validation,
     deferred diagnostics, source precedence, and save behavior operate on
     explicit models.
   - Red 2 tests: audio setup, display setup, scene defaults, UI editing, and
     save-on-exit consume only their configuration slices rather than globals or
     whole-application config.
   - Green targets: split runtime `Option` state from startup config, narrow
     `PathConfig`, keep source strategies testable, keep config slices
     read-only, and move persistence behind explicit module serializers.
   - Done since 73fd509: startup config acquisition/provisioning is typed,
     tested, and slice-based; legacy parser globals and no-arg ini write paths
     were removed.
   - Remaining: finish module-owned serializers for Scene/Display and reduce
     Configuration knowledge of runtime-owned domains.
   - Recheck target: no scalar/string startup option globals, path globals, or
     parser-owned globals outside Configuration and persistence adapters.

3. Runtime Reconfiguration - substantially complete
   - Done since 73fd509: runtime commands, mediator, subsystem control ports,
     runtime config registry, runtime persistence, shutdown, audio-processing,
     auto-change, effect-control, display, and mixer routing are in place.
   - Remaining: `Option`/`EffectControl` still own too much domain state for
     Scene and Display; those modules need owned live state before this module
     can become mostly metadata plus typed dispatch.
   - Recheck target: `Option` no longer owns parsing, validation, live state,
     UI text, side effects, and persistence as one object; live changes are not
     applied by writing globals or mutating immutable startup `Config`.

4. Commands And Input - interface runtime partial, keymap/input registries remaining
   - API first: `InputQueue`, `CommandRegistry`, `CommandDispatcher`,
     `CommandContext`, input-to-command keymaps, and option-editing state.
   - Red 1 tests: keymaps translate events into command intents without owning
     command targets; dispatcher executes against explicit command context.
   - Red 2 tests: key actions, option editing, help/credits, and X11 events no
     longer touch hidden current state or runtime globals.
   - Green targets: split `KeymapRegistry`, split `InterfaceManager`, explicit
     command registration, and UI edit state independent of display overlays.
   - Done since 73fd509: `InterfaceRuntime` owns interface selection and
     scoped option/effect command context.
   - Remaining: split static keymap/action registries and raw X11 key globals
     into explicit input queue/command registry/dispatcher.
   - Recheck target: no `x11_key`, `key_esc`, `Action::head`,
     `Keymap::first`, `Keymap::current`, `Interface::head`,
     `Interface::current`, `currentOption`, `currentEffectControl`, or static
     keymap dispatch consumers.

5. Scene - partially complete, still a major remaining module
   - API first: `SceneController`, `SceneState`, `SceneSnapshot`,
     `SceneChangeScheduler`, `SceneSerializer`, `EffectRegistry`, and
     `VisualCatalogs`.
   - Red 1 tests: scene state, catalog resolution, effect changes, presets, and
     auto-change policy run against fixture catalogs/options/audio snapshots.
   - Red 2 tests: `SceneCommands`, key actions, UI lists, preset loading, and
     auto-change no longer read visual option globals or `videoDirector()`.
   - Green targets: `EffectRegistry`, `VisualCatalogs`, scene selection state,
     scene serializer, and `AutoChanger` as Scene-owned
     `SceneChangeScheduler`.
   - Done since 73fd509: startup scene config is slice-based; `Scene` and
     `SceneCommands` exist; auto-change settings are owned; `autoChanger` is no
     longer a process-wide pointer.
   - Remaining: own visual selections, effect registry/catalogs, presets,
     scene serialization, and move AutoChanger policy out of
     `AudioVisualBridge`.
   - Recheck target: no visual selection globals, `EffectControl::first`,
     hidden preset traversal, `sceneCommandsForLegacyCallbacks()`, or global
     `autoChanger`.

6. Audio - substantially complete, with bridge/policy cleanup remaining
   - API first: `AudioAcquisitionRuntime`, `AudioPassthrough`, raw
     `AudioFrameProvider`, `AudioProcessingPipeline`, `AudioFrameProducts`,
     `AudioAnalysisSnapshot`, mixer/device adapters, and `AudioDumpWriter`.
   - Red 1 tests: audio runtime, frame provider, processor, analyzer, silent
     fallback, mixer/device config, and dump writer are independently fakeable.
   - Red 2 tests: `AudioVisualBridge`, `AudioProcessor`, `AudioAnalyzer`,
     screen contexts, and border effects consume explicit audio products.
   - Green targets: split `AudioSystem`, raw-only `AudioFrameProvider`,
     policy-free `AudioAnalysisState`, owned audio processor state, mixer state,
     and dump writer.
   - Done since 73fd509: `AudioRuntime` and audio frame facades were removed;
     acquisition, passthrough, processing selection, processor/FFT state,
     analysis metrics, output config/dump, Pulse diagnostics, and OSS mixer are
     owned/tested units.
   - Remaining: replace `AudioVisualBridge` with a cleaner audio pipeline and
     Scene-owned auto-change policy; remove process clock/random/logging
     dependencies as part of Application Lifecycle.
   - Recheck target: no audio facade functions, `audioAnalyzer`,
     `audioMetrics`, `acousticContext`, `AudioRuntime.cc` file-scope runtime
     state, mixer globals, or audio dump globals outside owners.

7. Frame Mutation - partly advanced by display/audio context work, still open
   - API first: `FrameStore`, `FrameComposer`, `FrameMutationContext`,
     renderer state factories, render math tables, and explicit frame handoff.
   - Red 1 tests: renderers and composers mutate fixture frames from explicit
     scene/audio/display-dimension snapshots.
   - Red 2 tests: `display.cc`, border, translation/image generation, and
     screen render paths no longer read global buffers, audio facades, or scene
     options.
   - Green targets: `CthughaBuffer` as `FrameStore`, renderer-local state,
     `ScreenRenderContext` audio/display inputs, and split-out composer pieces
     from `VideoDirector`.
   - Recheck target: no `CthughaBuffer::buffer`, `CthughaBuffer::current`,
     `display.cc` renderer statics, direct audio globals, or mutable math table
     globals outside explicit renderer/math owners.

8. Display - still open
   - API first: `DisplaySystem`, `DisplayBackend`, `DisplayGeometry`,
     native-pixel transfer, `OverlayMessageQueue`, `OverlaySink`, and raw event
     output.
   - Red 1 tests: fake display systems can open, report geometry, collect raw
     events, accept frames, and render overlays.
   - Red 2 tests: `Application`, frame presenter, overlays, X11 callbacks, and
     display commands use explicit display collaborators.
   - Green targets: split `DisplayRuntimeOwnership`, remove overlay device
     swapping, move native pixel tables/geometry/text state into display-owned
     objects, and route raw events to Commands And Input.
   - Recheck target: no `cthughaDisplay`, `displayDevice`, `displayBackend`,
     `displayRuntime`, X11 globals, global display geometry, global overlay
     text/error state, or logger-to-overlay back references.

9. Final compatibility removal
   - API first: none; this is a deletion pass backed by source-boundary tests.
   - Red tests: forbidden-token tests cover every removed global and adapter.
   - Green targets: delete compatibility aliases, stale declarations, and
     hidden singleton helpers.
   - Recheck target: every remaining global is immutable, file-local, or owned
     behind a named module boundary with contract and dependent tests.

### Completion Criteria

- Production code has no direct references to mutable compatibility globals:
  `cthugha_close`, `cthughaDisplay`, `displayDevice`, `displayBackend`,
  `displayRuntime`, `CthughaBuffer::buffer`, `CthughaBuffer::current`,
  `videoDirector()`, `autoChanger`, `EffectControl::firstRegistered()`,
  `Interface::current`, `Keymap::action(...)`, global audio facade functions,
  global audio metrics, global display geometry, and mutable option globals.
- Remaining global data is immutable, file-local, or hidden inside a named
  module boundary that has contract tests and dependent injection tests.
- `ApplicationContext` construction order is documented in `Application` tests:
  process services, options/paths/ini, registries/catalogs, buffer/display,
  audio/analysis, video/UI/input, lifecycle teardown.

Each module/component migration should preserve the existing tests:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
git diff --check
```
