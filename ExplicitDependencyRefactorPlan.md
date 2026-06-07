# Explicit Dependency Refactor Plan

## Goal

Make every runtime dependency explicit. Production code should receive the
state and services it needs through constructors, method parameters, or owned
context objects. Mutable process-wide globals should disappear from normal
application code. Temporary compatibility aliases are acceptable only when they
have a named removal phase and a source-boundary test that prevents new call
sites.

This plan covers production `src/` code. The display refactor has already
moved much of screen composition toward explicit `IndexedFrame`,
`ScreenRenderContext`, `PresentationComposer`, and `DisplayRuntime` handoffs.
Startup configuration acquisition is also now explicit:
`ConfigurationBuilder` builds a typed startup `Config`, and `Application`
passes immutable slices to subsystem startup APIs.

The remaining work is no longer one giant globals cleanup. It is a set of
module-boundary closures. Configuration, Runtime Reconfiguration, Commands and
Input, Audio, and Application Lifecycle all have substantial explicit
ownership in place. Scene, Frame Mutation, Display, and the diagnostics tail of
Process Services still contain the largest runtime-domain globals.

## Target Shape

`Application` should remain the composition root. It may know how to build all
module roots and wire them together, but normal production code should receive
only the narrow collaborator it needs. An `ApplicationContext` or similar
composition helper is allowed only as wiring; it must not become a service
locator.

The module boundaries are:

- Application Lifecycle
- Configuration
- Runtime Reconfiguration
- Commands And Input
- Scene
- Audio
- Frame Mutation
- Display

Each module should have an owned root or a small set of owned roots, explicit
ports for other modules, and tests that protect the boundary. For example, the
current `AutoChanger` behavior should become Scene-owned
`SceneChangeScheduler` policy that receives clock, random source, auto-change
settings, audio analysis snapshots, and a scene command target. It should not
read global audio metrics, global time, global random, display state, or a
director singleton.

## Module Boundaries

### Application Lifecycle

Owns process startup, run-loop sequencing, shutdown, platform lifecycle hooks,
construction/destruction order, process-level clocks/randomness/logging, and
frame pacing.

It must not own visual catalogs, mutable scene state, display backend/device
state, audio frame storage, command registries, or render buffers except as
module roots that it constructs and calls through explicit APIs.

### Configuration

Owns defaults, command-line parsing, ini loading, environment/config source
merging, validation, path resolution, startup schemas/descriptors, and
persistence format coordination.

It must not mutate downstream runtime state while parsing. Runtime selections
after startup are module state, not a mutable `Config`.

### Runtime Reconfiguration

Owns live setting changes after startup. It turns UI edits, key actions, panel
changes, automatic changes, and save/restore commands into typed requests
applied to the module that owns the live value.

It coordinates changes but does not own all runtime state. Scene owns scene
selection, Display owns presentation/display values, Audio owns audio runtime
values, and Application owns lifecycle-only toggles.

### Commands And Input

Owns raw input normalization, input queues, keymap lookup, command definition
registration, command dispatch, active interface state, and option-editing
interaction state.

It must not own scene/display/audio/shutdown state. Those are command targets
supplied through a per-dispatch `CommandContext`.

### Scene

Owns current visual scene state, effect/control registration, visual catalog
resolution, preset traversal, scene serialization, semantic cues, and automatic
scene-change policy.

Scene should describe what visual state is selected. It should not mutate
frame buffers, present frames, own display geometry, or execute filterchain
rendering.

### Audio

Owns audio acquisition, generated/file/live PCM sources, passthrough/output,
audio processing for visuals, acoustic analysis, audio device/mixer state, and
audio dump writing.

Audio produces explicit frame and analysis products. It must not own scene
policy, display presentation, render buffers, or auto-change decisions.

### Frame Mutation

Owns indexed frame storage and renderer-local mutation state. It turns scene
snapshots plus audio/frame inputs into pixels.

It should not own platform display backends, input command dispatch, runtime
persistence, or scene selection policy.

### Display

Owns platform/window backends, display devices, native pixel transfer,
geometry, overlays as rendered output, raw event collection, and frame
presentation.

It may publish raw input events to Commands/Input through an input sink, but it
must not dispatch commands or mutate scene/audio/display options through
globals.

## Module Plans

### Application Lifecycle

#### Current State

`Application` is the real composition root. It owns process services
(`SystemFrameSleeper`, `FramePacer`, `SystemMillisecondClock`,
`SystemSecondsClock`, `SystemCountdownTimerFactory`, `CStdRandomSource`,
`LoggingRuntime`, and `ConsoleLogSink`) plus the major runtime roots:
configuration, input/commands, scene, runtime reconfiguration, audio,
video/filterchain, display, mixer, and platform lifecycle.

Lifecycle ownership is mostly explicit:

- shutdown requests flow through `RuntimeShutdown`/`RuntimeCloseState`;
- suspend/resume hooks are owned by `PlatformLifecycle`;
- frame pacing uses the Application-owned `FramePacer` and seconds clock;
- migrated runtime paths receive clocks and random source through constructors
  or startup wiring;
- `CthughaDisplay` owns frame timestamp/delta state instead of exporting
  `now`/`deltaT`;
- logging verbosity lives in Application-owned `LoggingRuntime`.

Remaining lifecycle-adjacent debt:

- `CTH_*` macros still use `LegacyLoggingAdapter.cc` while older modules move
  to explicit `LogSink&`;
- Application still owns `AutoChanger`, even though its policy belongs under
  Scene as `SceneChangeScheduler`;
- Display compatibility aliases are still installed from Application
  (`cthughaDisplay`, `displayDevice`, `displayBackend`, and `displayRuntime`).

#### Services Needed

- `RuntimeShutdown` / `RuntimeCloseState`: shutdown request state and reason.
- `PlatformLifecycle`: signal/suspend/resume ownership and frame-boundary
  service hooks.
- `SecondsClock`, `MillisecondClock`, and `CountdownTimerFactory`: explicit
  time and deadline providers.
- `RandomSource`: deterministic source of runtime randomness.
- `LoggingRuntime` and `LogSink`: verbosity and diagnostics output.
- `FramePacer`: frame-rate limiting around the main loop.
- Module-root factories: narrow construction helpers for Scene, Audio,
  Commands/Input, Frame Mutation, and Display as those roots appear.

#### API Surface

Application-facing lifecycle surface:

```c++
class Application {
public:
  int initialize();
  void run();
  void shutdown();
  int exitStatus() const;
};
```

Process-service ports exposed to modules should remain narrow:

- `SecondsClock::nowSeconds()`
- `MillisecondClock::milliseconds()`
- `CountdownTimerFactory::makeTimer(...)`
- `RandomSource` random-value methods
- `LogSink::trace/debug/info/warn/error(...)`
- `RuntimeShutdown::requestClose()` and `closeRequested()`
- `PlatformLifecycle::install()`, `serviceFrameBoundary()`, and `shutdown()`

Modules should not receive `Application&` or a whole application context.

#### Migration Plan

1. Keep `Application` as the only object that can see the full composition
   graph.
2. As module roots land, move related fields out of `Application` into those
   roots without changing run-loop order.
3. Move Application-owned `AutoChanger` construction into Scene as
   `SceneChangeScheduler`; leave Application responsible only for calling the
   scheduler at the correct frame boundary.
4. Continue replacing `CTH_*` call sites with explicit `LogSink&` per module.
   Delete `LegacyLoggingAdapter.cc` only after production callers no longer
   need the macro bridge.
5. Delete display alias publication when the Display module owns backend,
   device, facade, and overlay paths behind explicit ports.
6. Keep source-boundary tests blocking reintroduction of process-global time,
   random, shutdown, lifecycle, and logging state.

### Configuration

#### Current State

Startup configuration acquisition is explicit and can be treated as complete
for the current boundary. `buildStartupConfig()` returns a
`ConfigBuildResult`; `ConfigurationBuilder` merges defaults, ini files,
environment values, and command-line patches into a typed `Config`; and
`Application` passes slices such as `AudioConfig`, `DisplayConfig`,
`SceneConfig`, `InputConfig`, `PathConfig`, `EffectPolicy`, and
`MessagesConfig` to owning subsystems.

Runtime persistence is no longer a no-argument global writer. Runtime save
commands route through `RuntimePersistence`/`IniRuntimePersistence` and read
current values through `RuntimeConfigRegistry` contributors. Ini writing uses a
local writer object and explicit `LogSink&`.

Remaining configuration debt is mostly ownership clarity:

- Configuration still defines keys and schemas for domains whose live state is
  still legacy `Option`/`EffectControl` state.
- Scene and Display still need module-owned serializers before Configuration
  is fully isolated from live runtime traversal.
- Some startup field names, such as presentation inside `SceneConfig`, still
  reflect historical ownership even when Application now routes the value to
  Display-side startup logic.

#### Services Needed

- `ConfigurationBuilder`: owns source order and precedence.
- `ConfigAcquisitionStrategy` / config sources: defaults, ini, environment,
  command line, and any bootstrap source.
- `ConfigPatch`: typed source contribution with diagnostics.
- `ConfigSchema`: option names, aliases, types, validation, canonical names,
  and deprecation warnings.
- `ConfigDiagnostic` and `DeferredLogBuffer`: structured diagnostics before
  logging configuration is finalized.
- `PathConfig` / future `PathResolver`: resolved resource and file paths.
- `ConfigPersistence` / `RuntimePersistence`: save coordination.
- Module serializers: `SceneSerializer`, `DisplaySerializer`,
  `AudioSerializer`, and any Application/Logging contributors.

#### API Surface

Startup acquisition:

```c++
ConfigBuildResult buildStartupConfig(int argc, char* argv[]);

ConfigurationBuilder builder;
ConfigBuildResult result = builder
  .addDefaults()
  .addIniFile(...)
  .addEnvironmentVariables(...)
  .addCommandLine(argc, argv)
  .build();
```

Consumers should receive slices, not the whole configuration:

- `const AppConfig&`
- `const LoggingConfig&`
- `const PathConfig&`
- `const AudioConfig&`
- `const DisplayConfig&`
- `const SceneConfig&`
- `const EffectPolicy&`
- `const SceneTransitionPolicy&`
- `const MessagesConfig&`
- `const InputConfig&`

Runtime persistence surface:

- `RuntimeConfigRegistry::addContributor(...)`
- `RuntimePersistence::writeCurrentConfig()`
- module-owned contributor/serializer interfaces that return current state
  snapshots.

#### Migration Plan

1. Keep startup `Config` immutable after `build()`.
2. Move live runtime values out of legacy options as Scene and Display get
   owned state.
3. Replace `LegacyRuntimeConfigContributor` with module contributors:
   Scene contributes scene selections, Display contributes presentation and
   display values, Audio contributes audio-processing/runtime values, and
   Application contributes lifecycle/config-save toggles.
4. Add `SceneSerializer` and `DisplaySerializer` so persistence does not walk
   `EffectControl::firstRegistered()` or global display options.
5. Keep source strategies pure: they may produce `ConfigPatch` values and
   diagnostics, but must not initialize buffers, open devices, load catalogs,
   mutate options, or emit runtime commands.
6. Preserve boundary tests that prove consumers can be constructed with only
   their config slice.

### Runtime Reconfiguration

#### Current State

Runtime command routing is explicit for the current code shape.
`RuntimeCommand` and `RuntimeCommandSink` define the command boundary.
`RuntimeChangeMediator` delegates to explicit ports for scene, display, audio,
auto-change, effects, persistence, and shutdown. `RuntimeConfigRegistry`
collects current snapshots for UI status and persistence.

The raw `Option&`/`EffectControl&` payloads were removed from
`RuntimeCommand`. Interface and X11 panel code now use
`RuntimeCommandTargetRouter`, which creates target-backed commands for
synchronous mediator calls.

Remaining debt:

- `Option` and `EffectControl` still carry too many responsibilities:
  live value, parsing, UI text, lock/use state, side effects, and persistence.
- Routed option/effect targets still adapt legacy objects rather than typed
  module-owned state.
- Scene and Display need owned live state before Runtime Reconfiguration can
  become mostly metadata plus typed dispatch.

#### Services Needed

- `RuntimeCommand`: typed command value and factory methods.
- `RuntimeCommandSink`: narrow command application port.
- `RuntimeChangeMediator`: central dispatcher over module ports.
- `RuntimeDisplayControls`, `RuntimeAudioControls`,
  `RuntimeAutoChangeControls`, and `RuntimeEffectControls`.
- `RuntimePersistence` and `RuntimeShutdown`.
- `RuntimeConfigRegistry` and module contributors.
- `RuntimeOptionTarget` and `RuntimeEffectControlTarget` while legacy options
  still need target-backed routing.
- Future typed module targets, such as `SceneCommandTarget` and
  `DisplayCommandTarget`.

#### API Surface

Primary command flow:

```c++
RuntimeChangeSet RuntimeCommandSink::apply(const RuntimeCommand& command);
```

Command construction stays value-oriented:

- `RuntimeCommand::changeSceneBy(...)`
- `RuntimeCommand::changeSceneTo(...)`
- `RuntimeCommand::changeScreenBy(...)`
- `RuntimeCommand::changeZoomBy(...)`
- `RuntimeCommand::changeSoundProcessingBy(...)`
- `RuntimeCommand::toggleAutoChangeLock()`
- `RuntimeCommand::writeIni()`
- `RuntimeCommand::requestClose()`
- target-backed option/effect command factories while migration adapters
  remain.

Subsystem ports should expose verbs, not raw state:

- `RuntimeDisplayControls::changeScreen...`, `changeZoom...`,
  `toggleShowFps()`
- `RuntimeAudioControls::changeSoundProcessing...`
- `RuntimeAutoChangeControls::toggleLock()`
- `SceneCommands` or future `SceneCommandTarget` scene-selection verbs
- `RuntimePersistence::writeCurrentConfig()`
- `RuntimeShutdown::requestClose()`

#### Migration Plan

1. Keep the mediator thin: it should translate commands to module ports, not
   own state or perform hidden global writes.
2. As Scene gets `EffectRegistry`, `VisualCatalogs`, and `SceneState`, replace
   routed legacy effect-control targets with typed Scene targets.
3. As Display gets `DisplaySystem`/`DisplayGeometry`/presentation state,
   replace routed display option writes with typed Display commands.
4. Replace `LegacyRuntimeConfigContributor` with module serializers and
   contributors.
5. Shrink `Option` and `EffectControl` toward metadata/adapters only. Live
   state should belong to Scene, Display, Audio, or Application.
6. Keep boundary tests blocking direct mediator writes to legacy globals or
   immutable startup `Config`.

### Commands And Input

#### Current State

Commands/Input has completed the concrete globals-removal phase.
`Application` now owns:

- `InputQueue`
- `CommandRegistry`
- `CommandDispatcher`
- `KeymapRegistry`
- `InterfaceRuntime`

Raw platform key events flow into an `InputEventSink`/`InputQueue`.
`KeyTranslator` owns ESC and shifted-key translation policy from
`InputConfig`. `Action::head`, `Keymap::first`, `Keymap::current`, static
keymap loading/name dispatch, direct keymap action execution, `x11_key`,
`key_esc`, key association globals, and public static interface panels have
been removed from production code.

Keymaps now store action names and produce `ActionInvocation` values.
`CommandDispatcher` executes invocations through an explicit
`CommandContext`. Interface selection, panel instances, extra-keymap state,
help/credits state, runtime adapters, and scoped option/effect edit context
live in `InterfaceRuntime` or the dispatch context.

Remaining debt:

- These objects are still individually owned and wired by `Application`;
  there is no coherent Commands/Input module root yet.
- Interface display code still reaches display globals such as
  `displayDevice`/`cthughaDisplay` while the Display module remains open.
- Additional behavior tests are still useful around keymap parsing, default
  binding loading, multi-action bindings, and direct help/list actions.

#### Services Needed

- `CommandsInputRuntime` or `InputCommandRuntime`: module root over the
  concrete collaborators.
- `InputQueue` and `KeyTranslator`: input normalization and FIFO storage.
- `CommandRegistry`: built-in action definitions.
- `KeymapRegistry`: named keymap storage and action lookup.
- `CommandDispatcher`: invocation execution.
- `CommandContext`: per-dispatch command targets and scoped edit target.
- `InterfaceRuntime`: active interface, owned panels, edit state, status/save
  flags, and panel-owned state.
- `InterfaceOverlaySource` / `OverlayProducer`: future Display-facing overlay
  port.

#### API Surface

A plausible module root surface:

```c++
class CommandsInputRuntime {
public:
  void configureInput(const InputConfig& config);
  void registerBuiltins();
  void loadKeymaps(const InputConfig& config, const PathConfig& paths);

  InputEventSink& inputSink();

  void runPendingInput(RuntimeCommandSink& runtimeCommands,
                       RuntimeCommandTargetRouter& targets);
  void runCurrentInterface(RuntimeCommandSink& runtimeCommands,
                           RuntimeCommandTargetRouter& targets);

  void selectInterface(const char* name);
  Interface* findInterface(const char* name);
  InterfaceOverlaySource& overlaySource();
};
```

Temporary accessors such as `findInterface("Mixer")` are acceptable during
mixer setup. They should stay narrow and have a deletion condition once Mixer
also has a cleaner setup port.

#### Migration Plan

1. Add focused behavior tests for keymap parsing/default loading,
   multi-action bindings, help/list actions, and any remaining edge cases.
2. Move `InputQueue`, `CommandRegistry`, `CommandDispatcher`,
   `KeymapRegistry`, and `InterfaceRuntime` fields from `Application` into a
   `CommandsInputRuntime` without changing behavior.
3. Move built-in action and interface registration behind
   `CommandsInputRuntime::registerBuiltins()`.
4. Replace event-loop call sites with `inputSink()`,
   `runPendingInput(...)`, and `runCurrentInterface(...)`.
5. Provide Display with an overlay source instead of having interface code
   print through global display aliases.
6. Make concrete registries and panel instances private implementation
   details of the module root, except in test fixtures.
7. Update boundary tests from "Application owns these concrete parts" to
   "production customers use the module root or narrow ports."

### Scene

#### Current State

`Scene` now owns the current scene settings, versioning, observer
notification, and semantic cues. `SceneCommands` is an explicit command facade
over `SceneCommandDependencies`; it no longer directly searches for most
legacy collaborators. Production wiring passes:

- flame, general-flame, wave, wave-scale, table, object, translation, palette,
  border, flashlight, and image controls;
- `SceneWaveObjectSource` for the current wave object;
- `SceneEffectRegistry` for save/restore/change-one/change-all operations;
- `EffectPresetCatalog`;
- `ScenePaletteRandomizer`;
- `CthughaBuffer&` for current geometry;
- Application-owned `RandomSource&`.

Scene-supporting cleanup already landed:

- display presentation startup selection moved out of
  `SceneCommands::applyStartupConfig(...)`;
- `Application` owns `VideoDirector`;
- the `videoDirector()` singleton accessor is gone;
- the `sceneCommandsForLegacyCallbacks()` bridge is gone;
- image loading and UI registration receive `ImageOption&` explicitly.

Remaining Scene debt is the biggest runtime-domain cluster:

- visual selection state still lives in global `EffectControl`/option objects
  such as `flame`, `wave`, `translation`, `palette`, `border`, and
  `flashlight`;
- `EffectControl::first` / `firstRegistered()` is still the hidden registry
  for randomization, save/restore, policy, UI lists, and presets;
- `EffectPresetCatalog` still traverses the hidden effect registry;
- `EffectControlPolicy` still applies policy by walking the hidden registry;
- visual catalogs and lists remain process-wide;
- `SceneCommands` still needs `CthughaBuffer::buffer` geometry through an
  explicit but legacy `CthughaBuffer&`;
- `AutoChanger` is Application-owned and should move to Scene support.

#### Services Needed

- `SceneState` / existing `Scene`: current settings, version, observers, and
  semantic cues.
- `SceneSnapshot`: immutable per-frame view of selected scene state.
- `SceneCommandTarget` / evolved `SceneCommands`: command API over scene-owned
  state.
- `EffectRegistry`: owned registry of scene-editable controls.
- `VisualCatalogs`: named flame/wave/table/object/translation/palette/image
  catalogs and allowed-choice resolution.
- `EffectPolicyApplier`: applies startup `EffectPolicy` to an explicit
  registry/catalog set.
- Owned `EffectPresetCatalog`: presets over an explicit `EffectRegistry`.
- `ScenePaletteRandomizer`: catalog-owned palette randomization.
- `SceneGeometry` or `FrameGeometry`: buffer dimensions needed for wave/image
  selection without depending on `CthughaBuffer::buffer`.
- `SceneSerializer`: current Scene persistence.
- `SceneChangeScheduler`: Scene-owned automatic change policy.

#### API Surface

Existing state API should remain small:

```c++
class Scene {
public:
  const SceneSettings& settings() const;
  unsigned int version() const;
  void setSettings(const SceneSettings& settings,
                   unsigned int forcedChanges = 0);
  void emitImageCue(const IndexedImage* image);
  void emitTextCue(const char* text, int frameCount, int inkColor);
  void addObserver(SceneObserver& observer);
  void removeObserver(SceneObserver& observer);
};
```

Command surface should evolve from legacy controls toward scene-owned targets:

```c++
class SceneCommandTarget {
public:
  void applyStartupConfig(const SceneConfig& config);
  void change(SceneSelectionTarget target, int by);
  void change(SceneSelectionTarget target, const char* to);
  void changeAll();
  void changeOne();
  void restore();
  void savePreset(int slot);
  void restorePreset(int slot);
  SceneSnapshot snapshot() const;
};
```

Scheduler surface:

```c++
class SceneChangeScheduler {
public:
  void run(const AudioAnalysisSnapshot& audio,
           SceneCommandTarget& sceneCommands);
  const char* statusText() const;
};
```

#### Migration Plan

1. Keep the current `SceneCommandDependencies` adapters as the only allowed
   compatibility surface while replacing the internals.
2. Introduce `EffectRegistry` and register effect controls into it during
   Application/Scene startup instead of linking them through
   `EffectControl::first`.
3. Convert `EffectPresetCatalog` to receive `EffectRegistry&` rather than
   traversing `EffectControl::firstRegistered()`.
4. Convert `EffectControlPolicy` into an `EffectPolicyApplier` over
   `EffectPolicy`, `EffectRegistry&`, and the owned preset catalog.
5. Introduce `VisualCatalogs` for named entries and allowed-choice resolution.
   Scene should store selected references/names; catalogs decide availability
   and legality.
6. Replace `SceneCommands` reads/writes of legacy global visual controls with
   scene-owned selection state and catalog lookups.
7. Replace the explicit `CthughaBuffer&` dependency with a geometry provider
   from Frame Mutation.
8. Add `SceneSerializer` and replace `LegacyRuntimeConfigContributor` for
   scene selections.
9. Move Application-owned `AutoChanger` into Scene support as
   `SceneChangeScheduler`. It receives auto-change settings, audio analysis,
   clock, random source, quiet-message observer, scene command target, and
   log sink.
10. Keep boundary tests blocking new `videoDirector()`,
    `sceneCommandsForLegacyCallbacks()`, direct Scene mutation of `screen`,
    direct `EffectControl::firstRegistered()` traversal outside named
    adapters, and direct visual-option globals in Scene code.

### Audio

#### Current State

Audio runtime ownership is largely explicit. `AudioRuntime` and audio-frame
facade functions are gone. `Application` owns `AudioIngest`, which owns source
acquisition, decoded history, visual pacing, completion state, and the current
`AudioFrame`. `AudioPassthrough`, output backends, submitted-PCM diagnostics,
output dump state, Pulse/OSS output diagnostics, and OSS mixer state are owned
objects with explicit configuration and `LogSink&`.

Processing and analysis are explicit:

- `AudioProcessingState` and `AudioProcessingSelector` own sound-processing
  mode and use the Application-owned random source for random selection.
- `AudioProcessor` owns its processing/FFT collaborators.
- `AudioFramePipeline` / `DefaultAudioFramePipeline` process each frame,
  update frame metrics, and update the Application-owned `AcousticContext`.
- `VideoFrameContext` and `ScreenRenderContext` carry audio products into
  visual consumers.

Remaining audio-adjacent debt:

- `AutoChanger` still consumes `AcousticContext` and frame metrics from
  Application; the policy belongs under Scene.
- Audio ingest still receives visual frame-window sizing from
  `CthughaBuffer::buffer.maxDimension()` during Application startup. That
  should become a Frame Mutation geometry dependency.
- The broader legacy logging macro cleanup belongs to Process
  Services/Diagnostics, not Audio ownership.

#### Services Needed

- `PcmSource` implementations and `PcmSourceFactory`: generated/file/live PCM.
- `AudioIngest` or future `AudioAcquisitionRuntime`: current raw frame
  acquisition, decoded history, pacing, and completion.
- `AudioPassthrough`: optional output draining.
- `AudioOutput`, backend adapters, and `AudioOutputDump`.
- `AudioProcessingState`, `AudioProcessingSelector`, `AudioProcessor`, and
  `AudioFftProcessor`.
- `AudioFramePipeline` / future `AcousticContextTracker`: per-frame visual
  processing and acoustic analysis.
- `AudioAnalysisSnapshot`: immutable analysis product for Scene and Frame
  Mutation.
- `MixerDevice`, `MixerSession`, and `MixerControls`.
- Optional `AudioModule` root to hide the concrete graph from `Application`.

#### API Surface

Existing important surfaces:

```c++
class AudioIngest {
public:
  int start();
  void stop();
  void tick();
  AudioFrame& currentFrame();
  int complete() const;
};

class AudioFramePipeline {
public:
  virtual void processFrame(AudioFrame& frame) = 0;
  virtual AcousticContext& acousticContext() = 0;
};
```

Target module-root surface:

```c++
class AudioModule {
public:
  int start(const AudioConfig& config, const AudioGeometry& geometry);
  void stop();
  void tick();

  const AudioFrame& currentFrame() const;
  AudioAnalysisSnapshot analysisSnapshot() const;

  RuntimeAudioControls& runtimeControls();
  MixerControls* mixerControls();
};
```

#### Migration Plan

1. Keep Audio policy-free: it produces raw/processed frame products and
   analysis, but does not decide scene changes.
2. Move automatic scene-change consumption of audio products into Scene's
   `SceneChangeScheduler`.
3. Introduce `AudioAnalysisSnapshot` so Scene and Frame Mutation receive an
   immutable view rather than a mutable `AcousticContext&`.
4. Replace `CthughaBuffer::buffer.maxDimension()` in audio startup with a
   `FrameGeometry`/`AudioGeometry` value from Frame Mutation.
5. Optionally wrap `AudioIngest`, processing, analysis, passthrough, and mixer
   setup behind an `AudioModule` root once Scene and Frame geometry boundaries
   are clearer.
6. Keep boundary tests blocking audio facade functions, global audio metrics,
   global analyzer/context objects, mixer globals, dump globals, and direct
   audio-to-scene policy.

### Frame Mutation

#### Current State

Frame Mutation has explicit context handoffs but not explicit ownership.
`VideoFrameContext` and `ScreenRenderContext` carry audio metrics,
`AcousticContext`, frame timing, and frame views into render paths. The modern
filterchain can produce an `IndexedFrame` for `CthughaDisplay::present(...)`.

The old mutable frame core remains:

- `CthughaBuffer::buffer` and `CthughaBuffer::current` are static aliases;
- `VideoDirector` still configures filterchains around the global buffer;
- `CthughaDisplay` and display backends still read `CthughaBuffer::current`;
- `display.cc` keeps renderer state in file-scope/function-local statics
  such as `perm_lines`, `height_offset`, rotation/scale state, `zicks`, and
  animation counters;
- `VideoFilters.cc` has filterchain diagnostic throttling in a function-local
  static;
- mutable math/render lookup tables still live as process-wide initialization
  state.

#### Services Needed

- `FrameStore`: owned indexed frame buffers, dimensions, active/passive frame
  selection, and swap/clear operations.
- `FrameGeometry`: stable dimensions for Scene, Audio, and Display.
- `FrameMutationContext`: per-frame inputs: `SceneSnapshot`,
  audio products/analysis, timing, display dimensions, and random source if
  needed.
- `FrameComposer`: turns context into an `IndexedFrame`.
- `RendererRegistry` and `RendererStateFactory`: owned renderer instances and
  reset-on-scene/size-change behavior.
- `ScreenRenderContext`: existing explicit renderer input carrier.
- `MathTables`: owned immutable or render-owned lookup tables.

#### API Surface

Frame storage:

```c++
class FrameStore {
public:
  void resize(PixelSize size);
  PixelSize dimensions() const;
  IndexedFrame& currentFrame();
  IndexedFrame& nextFrame();
  void swapFrames();
  void clear();
};
```

Frame mutation:

```c++
class FrameComposer {
public:
  const IndexedFrame& render(const SceneSnapshot& scene,
                             const AudioFrame& audio,
                             const AudioAnalysisSnapshot& analysis,
                             const FrameTiming& timing);
};
```

#### Migration Plan

1. Introduce an owned `FrameStore` while keeping it behavior-compatible with
   `CthughaBuffer`.
2. Move `CthughaBuffer::buffer`/`current` users to explicit `FrameStore&`,
   `IndexedFrame&`, or frame views. Keep temporary aliases only behind
   boundary tests.
3. Replace Scene and Audio geometry reads from `CthughaBuffer::buffer` with
   `FrameGeometry`.
4. Move `display.cc` renderer statics into renderer-state objects owned by
   Frame Mutation.
5. Move filterchain diagnostic throttles into filter instances.
6. Split `VideoDirector`: Scene transition/cue policy remains with Scene or
   director support, while frame composition/configuration moves into
   `FrameComposer`.
7. Move mutable math/render tables into explicit render-owned state or make
   them immutable file-local tables.
8. Keep Display receiving completed frames, not mutable buffer globals.

### Display

#### Current State

Display has an explicit runtime object but still publishes legacy aliases.
`DisplayRuntimeOwnership` owns a `DisplayDevice`, `DisplayBackend`, and
`DisplayRuntime`; `Application` calls `publishAliases()` to set
`displayDevice`, `displayBackend`, and `displayRuntime`. `Application` also
sets the public `cthughaDisplay` pointer after constructing `CthughaDisplay`.

Display event collection is improved: `DisplayRuntime::processEvents(...)`
receives an `InputEventSink&`, so raw platform input can flow to
Commands/Input without key mailboxes.

Remaining Display debt:

- global aliases are still used by interface overlay/rendering paths;
- X11 backend/device state still has global or static platform state;
- display geometry and pixel-transfer values remain global or backend-wide
  mutable values;
- overlays still print through display globals in Interface code;
- `CthughaDisplay` and device backends still depend on `CthughaBuffer::current`
  for fallback presentation paths.

#### Services Needed

- `DisplaySystem`: module root owning backend, device, facade, geometry,
  overlays, and lifecycle.
- `DisplayRuntime`: event pumping and frame presentation.
- `DisplayBackend` and `DisplayDevice`: platform/native operations.
- `DisplayGeometry`: output size, viewport, pixel format, pitch, zoom, and
  frame-buffer sizing.
- `DisplayFrontendInitializer`: frontend/Xt startup port.
- `OverlaySink` / `OverlayMessageQueue`: display-owned overlay output.
- `InterfaceOverlaySource`: Commands/Input-provided overlay producer.
- `NativePixelTransfer`: indexed-to-native pixel conversion tables/state.

#### API Surface

Target display module:

```c++
class DisplaySystem {
public:
  int open(const DisplayConfig& config,
           DisplayFrontendInitializer& frontend,
           InputEventSink& input);
  DisplayEventStats processEvents();
  DisplayGeometry geometry() const;

  void present(const IndexedFrame& frame,
               const VideoFrameContext& context,
               InterfaceOverlaySource& overlays);

  OverlaySink& overlaySink();
  void close();
};
```

Existing `DisplayRuntime` should remain a lower-level implementation detail:

```c++
DisplayEventStats processEvents(InputEventSink& input);
void present(const IndexedDisplayFrame& frame,
             const DisplayViewport& viewport,
             int needsFullCopy,
             int needsBorderClear,
             const OverlayCommands& overlays);
```

#### Migration Plan

1. Split `DisplayRuntimeOwnership` into a real `DisplaySystem` root.
2. Stop publishing `displayDevice`, `displayBackend`, `displayRuntime`, and
   `cthughaDisplay`; pass narrow display/overlay/status collaborators instead.
3. Move Interface overlay drawing behind an `InterfaceOverlaySource` consumed
   by Display.
4. Move X11 globals and static platform state into the X11 backend/device
   objects.
5. Move geometry, pixel format, native color tables, and text/font state into
   `DisplayGeometry` and display-owned implementation objects.
6. Replace fallback reads of `CthughaBuffer::current` with explicit frame
   views from Frame Mutation.
7. Keep Display output-only with respect to commands: it publishes input
   events into `InputEventSink&`, but never dispatches runtime commands.

## Recommended Next Module

The next high-value module is Scene.

Commands/Input is ready for a polishing root, but Scene still owns the largest
runtime-domain dependency knot: visual selection globals, the hidden
`EffectControl` registry, preset traversal, catalog policy, Scene
serialization, and Application-owned auto-change policy. Closing Scene also
unblocks cleaner Runtime Reconfiguration, Audio policy separation, and the
Frame Mutation handoff.

Recommended Scene order:

1. Add `EffectRegistry` and move registry traversal out of
   `EffectControl::first`.
2. Move presets and effect policy onto the explicit registry.
3. Introduce `VisualCatalogs` and scene-owned selection state.
4. Add `SceneSerializer` and replace the legacy runtime config contributor.
5. Move `AutoChanger` into Scene as `SceneChangeScheduler`.
6. Replace the `CthughaBuffer&` Scene dependency with frame geometry.

## Cross-Cutting Guard Rails

- Every compatibility alias needs an owner file, a removal condition, and a
  source-boundary test.
- Prefer owned module roots over service-locator context objects.
- Prefer typed snapshots and command targets over generic `Option*` or
  `EffectControl*` payloads.
- Preserve current smoke coverage after each migration:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tests/headers/check-headers.sh
git diff --check
```

## Completion Criteria

- Production code has no direct references to mutable compatibility globals:
  `cthugha_close`, `cthughaDisplay`, `displayDevice`, `displayBackend`,
  `displayRuntime`, `CthughaBuffer::buffer`, `CthughaBuffer::current`,
  `EffectControl::firstRegistered()`, legacy audio facade functions, global
  audio metrics/context, visual selection globals, static keymap/action
  registries, or global display geometry.
- Remaining global data is immutable, file-local, or hidden inside a named
  module boundary with contract tests and dependent injection tests.
- `Application` construction order is documented by tests: process services,
  configuration, Commands/Input, Scene, Runtime Reconfiguration, Display/Frame
  geometry, Audio, Frame Mutation, presentation, and teardown.
