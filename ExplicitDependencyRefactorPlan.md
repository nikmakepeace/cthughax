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
Input, Audio, Scene, and Application Lifecycle all have substantial explicit
ownership in place. Frame Generator, Display, and the diagnostics tail of
Process Services still contain the largest runtime-domain globals. Scene's
remaining adapter edge is tied to the legacy visual catalog and frame generator
stack.

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
- Frame Generator
- Display

Each module should have an owned root or a small set of owned roots, explicit
ports for other modules, and tests that protect the boundary. The current
`SceneChangeScheduler` shape is the model: it receives clock, random source,
auto-change settings, diagnostics, and a scene command target instead of
reaching through process globals.

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

Owns current visual scene state, selection registry/presets, visual catalog
ports, scene serialization, semantic cues, and automatic scene-change policy.

Scene should describe what visual state is selected. It should not mutate
frame buffers, present frames, own display geometry, or execute filterchain
rendering.

### Audio

Owns audio acquisition, generated/file/live PCM sources, passthrough/output,
audio processing for visuals, acoustic analysis, audio device/mixer state, and
audio dump writing.

Audio produces explicit frame and analysis products. It must not own scene
policy, display presentation, render buffers, or auto-change decisions.

### Frame Generator

Owns indexed frame storage and renderer-local generation state. It turns scene
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
  Commands/Input, Frame Generator, and Display as those roots appear.

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
3. Continue replacing `CTH_*` call sites with explicit `LogSink&` per module.
   Delete `LegacyLoggingAdapter.cc` only after production callers no longer
   need the macro bridge.
4. Delete display alias publication when the Display module owns backend,
   device, facade, and overlay paths behind explicit ports.
5. Keep source-boundary tests blocking reintroduction of process-global time,
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
- Scene selections now contribute through `SceneSerializer`; display
  presentation, audio-processing, auto-change, and a few effect-policy toggles
  still pass through `LegacyRuntimeConfigContributor`.
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
- Module serializers: existing `SceneSerializer`, future `DisplaySerializer`,
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
   Display contributes presentation and display values, Audio contributes
   audio-processing/runtime values, and Application contributes lifecycle,
   auto-change, and config-save toggles.
4. Add `DisplaySerializer` and `AudioSerializer` so persistence does not walk
   global display/audio options.
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
- Display still needs owned live state before Runtime Reconfiguration can
  become mostly metadata plus typed dispatch. Scene command routing is typed,
  but legacy effect-control targets remain as adapters until native visual
  catalogs replace `LegacyScene*`.

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
- Existing `SceneCommandTarget` and future `DisplayCommandTarget`.

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
2. Retire routed legacy effect-control targets after native visual catalogs
   replace `LegacyScene*`.
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

`Scene` and `SceneRuntime` now have an explicit dependency boundary. Scene
commands, selection history, presets, startup effect policy, serialization, and
automatic scene-change policy are all owned by Scene-facing types.

The remaining Scene-facing debt is intentionally quarantined in `LegacyScene*`
adapters:

- `Application` still creates `createLegacySceneVisualCatalogFactory(...)` from
  global visual `EffectControl`/option objects such as `flame`, `wave`,
  `translation`, `palette`, `border`, and `flashlight`.
- `LegacySceneSelectionAdapters`, `LegacySceneVisualCatalogs`, and
  `LegacySceneEffectControlCatalog` translate those legacy controls into
  `SceneVisualSelections`, `SceneVisualCatalogs`, and runtime effect-control
  routing.
- The adapter remains necessary until the visual catalog/filterchain side owns
  flames, waves, palettes, images, translation tables, and frame geometry
  without `CthughaBuffer::buffer` or process-wide option catalogs.

#### Services Needed

- Native `SceneVisualCatalogFactory` / `SceneVisualSelections`: backed by owned
  visual catalog state rather than legacy `EffectControl` globals.
- Owned visual catalogs for flame, wave, table, object, translation, palette,
  image, border, and flashlight choices, including allowed-choice metadata.
- Native `ScenePaletteRandomizer` and wave-object source owned by the visual
  catalog/runtime boundary rather than `LegacySceneCatalogAdapters`.
- `FrameGeometry` from Frame Generator so Scene and visual catalogs do not depend
  on `VideoDirector`/`CthughaBuffer` as geometry providers.
- Removal-condition tests for deleting `LegacyScene*` once the native visual
  factory replaces `createLegacySceneVisualCatalogFactory(...)`.

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

Command surface is now scene-owned:

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

Desired scheduler handoff after the Audio analysis snapshot work:

```c++
class SceneChangeScheduler {
public:
  void run(const AudioAnalysisSnapshot& audio,
           SceneCommandTarget& sceneCommands);
  const char* statusText() const;
};
```

#### Migration Plan

1. Keep `LegacyScene*` as the only allowed compatibility surface while the
   visual/filterchain runtime is still legacy-backed.
2. Deglobalize Frame Generator: introduce owned frame storage/geometry and split
   `VideoDirector`/filterchain responsibilities away from `CthughaBuffer`
   aliases.
3. Introduce native visual catalogs and selection storage for flames, waves,
   translations, palettes, images, border, flashlight, and related metadata.
4. Replace `createLegacySceneVisualCatalogFactory(...)` in `Application` with
   the native `SceneVisualCatalogFactory`.
5. Delete `LegacyScene*` adapters and their boundary exceptions once no
   production wiring needs legacy visual `EffectControl&` inputs.

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

- `SceneChangeScheduler` still receives mutable `AcousticContext&` plus
  per-frame `AudioMetrics`; this should become an immutable
  `AudioAnalysisSnapshot` handoff.
- Audio ingest still receives visual frame-window sizing from
  `CthughaBuffer::buffer.maxDimension()` during Application startup. That
  should become a Frame Generator geometry dependency.
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
  Generator.
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
2. Introduce `AudioAnalysisSnapshot` so Scene and Frame Generator receive an
   immutable view rather than a mutable `AcousticContext&`.
3. Replace `CthughaBuffer::buffer.maxDimension()` in audio startup with a
   `FrameGeometry`/`AudioGeometry` value from Frame Generator.
4. Optionally wrap `AudioIngest`, processing, analysis, passthrough, and mixer
   setup behind an `AudioModule` root once Scene and Frame geometry boundaries
   are clearer.
5. Keep boundary tests blocking audio facade functions, global audio metrics,
   global analyzer/context objects, mixer globals, dump globals, and direct
   audio-to-scene policy.

### Frame Generator

#### Current State

Frame Generator is the new name for the old Frame Mutation/video-filterchain
module. Its responsibility is to turn an explicit scene snapshot, audio frame,
analysis state, timing, and generator-owned renderer state into one completed
`IndexedFrame`.

The module already has useful per-frame handoffs. `VideoFrameContext` carries
audio metrics, `AcousticContext`, frame timing, and a `SceneSnapshot` into video
filters. The filterchain can publish an `IndexedFrame` for
`CthughaDisplay::present(...)`, and filter-local state such as
`FlameLookupTables`, `WaveState`, and `WaveLookupTables` is already object-owned.

The remaining implicit dependencies are:

- `CthughaBuffer::buffer` and `CthughaBuffer::current` are static aliases rather
  than owned frame storage supplied by `Application`.
- `Application` configures dimensions, visual catalogs, audio ingest sizing, and
  filterchain execution through `CthughaBuffer::buffer`.
- `VideoDirector` mixes scene observation, quiet-message policy, image cue
  placement, palette-transition policy, filterchain configuration, frame-budget
  calculation, and target-buffer access.
- `VideoDirector.cc` keeps palette and quiet-message settings in file-scope
  statics, and reads `cthughaDisplay` to estimate frame budgets.
- `CthughaDisplay` and X11 display fallback paths still read
  `CthughaBuffer::current` when no explicit `IndexedFrame` is supplied.
- `VideoFilters.cc` has frame-commit diagnostic throttling in a function-local
  static.
- `imath.cc` publishes mutable sine tables initialized by `Application`, and
  wave renderers read those tables through process globals.
- `display.cc` keeps presentation renderer state in file-scope/function-local
  statics. That state belongs to Display, not Frame Generator, because it runs
  after a source `IndexedFrame` already exists.

#### Services Needed

- `FrameGeneratorRuntime`: Application-owned module root. Owns frame storage,
  filterchain/pipeline state, scene binding, frame-generation settings, and
  generator diagnostics.
- `FrameGeometry`: stable logical frame dimensions and derived values
  (`width`, `height`, hidden border rows, hidden border bytes,
  `maxDimension`). It implements or adapts to `SceneGeometry` but is not a
  buffer.
- `FrameStore`: owned indexed active/passive storage, hidden border storage,
  active/passive selection, clear, resize, and swap operations.
- `FrameGeneratorContext`: per-frame borrowed inputs: `SceneSnapshot`,
  `AudioFrame`, future `AudioAnalysisSnapshot` or current `AcousticContext`
  compatibility view, timing, and frame-rate budget values.
- `FrameGeneratorSceneBinding`: observes a `Scene` for changes and one-shot
  image/text cues. It queues generator work but does not own scene selection
  policy.
- `FrameFilterchainPipeline` / `FrameComposer`: owns the filterchain and
  converts a `FrameGeneratorContext` plus `FrameStore` into an `IndexedFrame`.
- `FrameTransitionController`: owns palette smoothing, image-cue palette mode,
  and other frame-generation transition settings currently hidden in
  `VideoDirector.cc` statics.
- `FrameMathTables`: immutable or generator-owned sine/angle lookup state used
  by wave renderers.
- `FrameGeneratorOutput`: an explicit result view whose pixels remain valid
  until the next generator render, resize, or shutdown.

#### API Surface

Frame geometry and storage:

```c++
class FrameGeometry : public SceneGeometry {
public:
  PixelSize size() const;
  int width() const;
  int height() const;
  int hiddenBorderRows() const;
  int hiddenBorderByteCount() const;
  int maxDimension() const;
};

class FrameStore {
public:
  void resize(const FrameGeometry& geometry);
  const FrameGeometry& geometry() const;
  FrameBufferView active();
  FrameBufferView passive();
  void swapActivePassive();
  void clear();
};
```

Frame generation:

```c++
class FrameGeneratorRuntime {
public:
  const FrameGeometry& geometry() const;
  SceneGeometry& sceneGeometry();

  void configure(const DisplayConfig& display,
                 const SceneTransitionPolicy& transitionPolicy);
  void bindScene(Scene& scene);
  void unbindScene();

  const IndexedFrame& render(const FrameGeneratorContext& context);
};
```

#### Migration Plan

The refactor is complete only when no production object in this module obtains
dependencies through `CthughaBuffer::buffer`, `CthughaBuffer::current`,
`cthughaDisplay`, display globals, or process-wide mutable math tables. All
runtime collaborators must be supplied by constructor, method parameter, or an
owned object created by `Application`.

1. **Rename the boundary before moving behavior.**
   Treat new code and tests as `FrameGenerator*`. Keep old filenames such as
   `VideoFilterchain` only as implementation details until a mechanical rename
   is safe. Update docs and boundary tests so "Frame Mutation" is a legacy term
   and "Frame Generator" is the module name.

2. **Add guard tests before production migration.**
   Add a source-boundary test for the new module that initially records the
   allowed compatibility files and then tightens after each phase. The final
   assertions must block:
   - `CthughaBuffer::buffer` and `CthughaBuffer::current` in production `src/`;
   - `cthughaDisplay`, `DisplayDevice`, `DisplayRuntime`, `RuntimeCommand*`,
     `RuntimePersistence`, `screen`, `zoom`, or display geometry globals from
     Frame Generator headers and implementation;
   - `CthughaBuffer::current` in `CthughaDisplay.cc` and X11 display code;
   - `static int debugReports` in `VideoFilters.cc`;
   - `extern int sine`, `sin360`, `init_imath()`, `isin()`, or `icos()` in wave
     generator code once `FrameMathTables` exists.

3. **Introduce `FrameGeometry` as a value and a narrow port.**
   Build `FrameGeometry` from startup `DisplayConfig.bufferWidth` and
   `bufferHeight`. Include hidden border rows because flame feedback relies on
   that storage, but do not expose pixels through geometry. `Application` should
   configure this immediately after startup config is built and before any
   subsystem needs visual dimensions.

4. **Wire geometry explicitly before storage changes.**
   Replace Scene geometry construction from `VideoDirector` with
   `frameGenerator.sceneGeometry()`. Replace Audio ingest sizing from
   `CthughaBuffer::buffer.maxDimension()` with
   `frameGenerator.geometry().maxDimension()` or a copied `AudioGeometry` value.
   Replace visual catalog and image-loading geometry reads with
   `FrameGeometry`. At the end of this phase, Scene and Audio know dimensions
   but cannot reach frame pixels.

5. **Introduce `FrameStore` behind behavior-compatible storage.**
   Make `FrameStore` own the active/passive indexed buffers and hidden rows.
   It may initially contain a `CthughaBuffer` implementation object so existing
   flame, translate, wave, border, image, and commit filters can still receive
   an injected buffer reference. The important rule is that the implementation
   object is owned by `FrameStore`; it is never reached through static aliases.
   Add tests for resize, hidden border offsets, active/passive swap semantics,
   clear semantics, and `IndexedFrame` publication.

6. **Create the Application-owned module root.**
   Add `FrameGeneratorRuntime` as an `Application` member or owned pointer with
   lifetime longer than Display and the filterchain output it presents.
   Construction dependencies are `RandomSource&`, `LogSink&`, and any clock or
   frame-budget provider needed by generator policy. Configuration dependencies
   are passed through `configure(...)`, not read from globals. Member order must
   guarantee:
   - process services outlive `FrameGeneratorRuntime`;
   - `FrameGeneratorRuntime` outlives `SceneRuntime` ports that borrow its
     `SceneGeometry`;
   - Display is destroyed before `FrameGeneratorRuntime`, because the last
     presented `IndexedFrame` may point into `FrameStore`;
   - scene binding is unbound before `SceneRuntime` is destroyed.

7. **Move filterchain ownership from `Application` into Frame Generator.**
   `Application` should stop owning `videoFilterchain` and
   `videoFilterchainSequence`. `FrameGeneratorRuntime` owns the factory,
   filterchain, frame palette, pending scene-change flags, and refresh/rebuild
   behavior. The render call becomes:
   - build a `FrameGeneratorContext` from explicit per-frame inputs;
   - apply queued scene changes and cues to the owned filterchain;
   - run the filterchain against `FrameStore`;
   - return an explicit `IndexedFrame` result.

8. **Split `VideoDirector` by responsibility, then delete it.**
   Move responsibilities to these owners:
   - `FrameGeneratorSceneBinding`: observes `Scene` changes and cues, queues
     image/text cue data, and marks filterchain settings dirty.
   - `FrameTransitionController`: owns palette smoothing settings and image cue
     palette behavior.
   - `FrameFilterchainConfigurator`: applies `SceneSnapshot.settings()` and
     queued cues to filter instances.
   - Scene or Runtime Reconfiguration: owns quiet-message policy and emits
     `SceneCue` text messages. `SilenceMessage`, `changeMsgTime`, and quiet
     duration state must not stay in Frame Generator.
   After the split, there should be no object that both observes Scene policy
   and owns target pixels.

9. **Make frame-budget and timing inputs explicit.**
   Remove `VideoDirector` reads of `cthughaDisplay->rollingFps`. Pass the
   current rolling FPS or a `FrameBudget` value through `FrameGeneratorContext`.
   Palette smoothing and text durations should use that explicit value. The
   generator must not include `CthughaDisplay.h` or any display backend header.

10. **Remove display fallback reads of generator storage.**
    Require `Application` to pass the `IndexedFrame` returned by
    `FrameGeneratorRuntime::render(...)` to Display. Remove the
    `presentCurrent(...)` fallback that lets `CthughaDisplay` read
    `CthughaBuffer::current`. Replace X11 fallback indexed-display dimensions
    with explicit presentation frame or viewport dimensions. Display may cache
    the last supplied frame view for the duration of presentation, but it must
    never ask Frame Generator for "current" mutable storage.

11. **Move generator diagnostics into owned objects.**
    Convert the `FrameCommitFilter` diagnostic throttle from a function-local
    static to a `FrameCommitFilter` member. Any future diagnostic counters live
    in the filter, `FrameFilterchainPipeline`, or `FrameGeneratorRuntime`, and
    receive logging through an explicit `LogSink&`.

12. **Move mutable math tables into generator-owned state.**
    Replace `Application::init_imath()` and global `sine`/`sin360` reads in
    wave generation with `FrameMathTables` supplied through `WaveRuntime` or
    `WaveLookupTables`. The resulting tables may be immutable after
    construction, but their owner must be explicit. Display-side math cleanup is
    separate unless a display renderer is reading the same global table.

13. **Hide or retire `CthughaBuffer`.**
    Once all production call sites receive `FrameStore&`, `FrameBufferView&`,
    or `IndexedFrame`, make `CthughaBuffer` a private compatibility
    implementation detail or rename it to the new storage type. Remove the
    static `buffer` and `current` declarations. Benchmarks/tests may create
    local stores explicitly, but production code must not have a process-wide
    buffer.

14. **Keep presentation renderer state in Display.**
    Do not move `display.cc` state such as `perm_lines`, height-field rotation,
    `screen_bent` animation values, or `zicks` into Frame Generator. Those are
    `ScreenRenderContext` presentation effects over an already generated
    source frame. Track them under the Display plan as display-owned renderer
    state.

15. **Tighten lifecycle tests at the end.**
    Add or update tests that prove:
    - `FrameGeneratorRuntime` can be constructed with explicit process services
      and configured with explicit startup values;
    - `SceneRuntime` receives only a `SceneGeometry&` or value, not frame
      storage;
    - Audio ingest receives only a geometry value;
    - one render borrows audio/context inputs only for the call;
    - the returned `IndexedFrame` remains valid until the next render, resize,
      or generator shutdown;
    - shutdown unbinds Scene before destroying Scene runtime and destroys
      Display before frame storage.

16. **Completion gate.**
    Run the normal build/test/check sequence plus the new boundary tests. The
    Frame Generator refactor is not complete until production `src/` has no
    direct references to `CthughaBuffer::buffer`, `CthughaBuffer::current`, or
    display globals, and the only remaining globals touched by generation code
    are immutable catalog definitions or temporary compatibility aliases with a
    named removal test.

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

The Display refactor is complete only when every Display object receives all
runtime state through its constructor, an open/configure request, a per-frame
method parameter, or another object owned by `DisplaySystem`. No production
Display path may reach through `displayDevice`, `displayBackend`,
`displayRuntime`, `cthughaDisplay`, `disp_size`, `bytes_per_line`, `bypp`,
`draw_mode`, `text_size`, `fontSize`, `bitmap_colors*`,
`CthughaBuffer::current`, or X11 file-scope configuration state.

1. **Lock the target boundary with tests first.**
   Add a Display source-boundary test that starts permissive and tightens after
   each phase. The final assertions must prove:
   - `Application` owns exactly one `DisplaySystem` root and does not call
     `cth_init`, `configureDisplayDeviceX11`, `newDisplayDevice`,
     `newCthughaDisplay`, or `publishAliases`;
   - `DisplayDevice.h`, `DisplayBackend.h`, `DisplayRuntime.h`, and
     `CthughaDisplay.h` no longer export mutable display aliases;
   - Interface files do not include `DisplayDevice.h` or `CthughaDisplay.h` and
     do not read `text_size`;
   - X11 code does not read `CthughaBuffer::current` or global display geometry;
   - Display code does not include Frame Generator implementation headers.

2. **Choose the driver strategy before splitting code.**
   Use compile-time configuration only to decide which driver implementations
   are available, and runtime configuration to select the active driver:
   - keep `CTH_BUILD_X11` for the existing X11 driver;
   - add `CTH_BUILD_SDL3` for the new SDL3 driver and find/link SDL3 only when
     that option is enabled;
   - add a typed `DisplayDriverId` to startup config with values
     `auto`, `x11`, and `sdl3`, parsed from an ini key such as
     `display.driver` and a command-line option such as `--display-driver`;
   - `auto` selects the first available compiled driver using a deterministic
     priority, initially X11 before SDL3 to preserve current behavior;
   - explicitly selecting an unavailable driver is a startup configuration
     error with a clear diagnostic.

3. **Introduce `DisplaySystem` without changing behavior.**
   Replace `DisplayRuntimeOwnership` with an owned root that contains, in
   explicit lifetime order, the driver-specific platform state, backend/device,
   runtime, and presentation coordinator. Shutdown must destroy those in the
   reverse dependency order: coordinator first, runtime next, backend/device
   next, platform state last. The root should expose only: `open(...)`,
   `processEvents(InputEventSink&)`, `present(...)`, `statusSnapshot()`,
   `geometry()`, `settings()`, and `close()`. `DisplayRuntime` may remain as an
   internal adapter over `DisplayBackend`, but it should not be published
   outside `DisplaySystem`.

4. **Move frame timing out of the display facade dependency chain.**
   Today `CthughaDisplay` owns `FrameClock`, and Audio/Frame Generator borrow
   timing by asking the display object. Make the dependency explicit:
   `Application` or an Application-owned `VisualFrameClock` begins the frame and
   produces a `FrameTiming`/`VideoFrameContext` value. Display receives that
   value for FPS/status overlays and presentation effects. Frame pacing and
   `maxFramesPerSecond` belong to Application Lifecycle/FramePacer settings;
   Display owns only presentation-facing state such as selected screen, zoom,
   overlay visibility, viewport, and presentation latency.

5. **Create the driver registry and factory seam.**
   Add a `DisplayDriverFactory` interface that can create a driver from a
   `DisplayOpenRequest`. `Application` builds a `DisplayDriverRegistry` from
   compiled-in factories and passes it plus startup config into
   `DisplaySystem::open(...)`. After this step the only compile-time `#ifdef`
   branches for X11/SDL3 selection should live in the registry construction or
   CMake, not in `Application` or the main loop.

6. **Fold frontend initialization into driver open.**
   Replace `DisplayFrontendInitializer`/`cth_init` with driver-local open
   logic. For X11 this means `XtAppInitialize`, `Display*`, `XtAppContext`, and
   the top-level widget become members of an `X11DisplayContext` owned by the
   X11 driver. SDL3 will initialize SDL video in its own driver object. Both
   drivers must close their platform frontend during `DisplaySystem::close()`.

7. **Replace compatibility aliases with explicit collaborators.**
   Remove `publishAliases()` in phases. First introduce explicit parameters for
   every current alias user:
   - `Application` calls `displaySystem.processEvents(inputQueueValue)`;
   - presentation code receives `DisplayRuntime&` or, preferably,
     `DisplayPresenter&` from `DisplaySystem`;
   - runtime display controls receive `DisplayPresentationSettings&`;
   - Interface overlay code receives an `OverlaySink&` and overlay layout;
   - frame-budget users receive `FrameTiming` or a `FrameRateSnapshot`.
   Keep a temporary `LegacyDisplayAliases` helper only if needed, with a test
   that names its owner and prevents new call sites. Delete it before the
   Display refactor is complete.

8. **Make per-frame data handoff one-way and explicit.**
   `Application` should pass the frame returned by Frame Generator directly to
   Display:
   `displaySystem.present(indexedFrame, presentationContext, overlaySource)`.
   Display may borrow the source `IndexedFrame` only for the duration of that
   call. It may own and reuse its composed `IndexedDisplayFrame` until the next
   present, resize, or shutdown. Backends receive only a `DisplayPresentation`
   value containing the composed indexed frame, viewport, copy/clear flags,
   palette pointer, and collected overlay commands. No Display object may ask
   Frame Generator for "current" pixels.

9. **Remove `CthughaBuffer::current` display fallbacks.**
   Delete `CthughaDisplay::sourcePixels()`, `sourceWidth()`, `sourceHeight()`,
   and `sourcePitch()` fallback reads once `Application` always presents an
   explicit frame. Replace X11 `fallbackIndexedDisplayWidth/Height()` with
   dimensions from the current `DisplayPresentation`, the last valid
   presentation frame, or an initial frame-size value from `DisplayOpenRequest`.
   The display module must fail closed or skip presentation on an invalid frame
   rather than reading global frame storage.

10. **Move presentation options into display-owned settings.**
    Replace global `screen`, `zoom`, and `showFPS` with a
    `DisplayPresentationSettings` object owned by `DisplaySystem`. Move
    `maxFramesPerSecond` to an Application Lifecycle/FramePacer settings object
    and wire any command that edits it through a separate lifecycle-facing port.
    Runtime reconfiguration receives a `RuntimeDisplayControls` implementation
    backed by display presentation settings. The selected screen/presentation
    catalog remains Display-owned, while Frame Generator receives no
    presentation option references.

11. **Turn `CthughaDisplay` into a backend-neutral presentation coordinator.**
    Rename or reshape it as `DisplayPresentationCoordinator`. It should own
    `PresentationComposer`, `IndexedDisplayFrame`, `DisplayViewport`,
    border-clear state, presentation latency, and display renderer state. It
    should depend on:
    - `DisplayPresentationSettings&`;
    - `DisplayOutput&`/`DisplayRuntime&` supplied by `DisplaySystem`;
    - explicit `FrameTiming`/`VideoFrameContext` supplied per frame;
    - an `InterfaceOverlaySource&` supplied per frame or at open time.
    It should not include `xcthugha.h`, call X11 methods, read display globals,
    or publish `cthughaDisplay`.

12. **Make overlays a real interface-to-display handoff.**
    Introduce `InterfaceOverlaySource` or extend `OverlaySource` so overlay
    producers render into an `OverlaySink&` with an explicit `OverlayLayout`
    containing text columns, rows, font size, and status-line positions.
    Replace the `ScopedOverlayDisplayDevice` trick in `CthughaDisplayX11.cc`:
    Interface and `ErrorMessages` produce `OverlayCommands`; Display renders
    those commands. `FpsOverlay` consumes a `DisplayStatusSnapshot` and the
    display settings flag, not `cthughaDisplay`.

13. **Move text metrics and text rendering state into Display.**
    Replace global `text_size` and `fontSize` with `DisplayTextMetrics`.
    Replace `DisplayDevice::textOnScreen`, `darkenPalette`, text colors, and
    text palette side effects with a display-owned `DisplayTextState` or
    backend text renderer. Interface code should use only `OverlayLayout`;
    X11 and SDL3 should use their backend renderer to draw collected overlay
    commands. X11 may continue to use server fonts; SDL3 should start with the
    existing `BitmapFont`/`dosVga9x14Font()` so no SDL_ttf dependency is needed
    for the first driver.

14. **Move native pixel transfer and palette state into backend objects.**
    Replace `bitmap_colors0..3`, `bypp`, `bytes_per_line`, `draw_mode`, and
    `rev_byte_order` with:
    - `DisplayPixelFormat` for bytes per pixel, pitch, masks, shifts, and byte
      order;
    - `NativePalette` or `NativePixelTable` for the 256 indexed-to-native
      values;
    - `DisplaySurface` for writable native pixels and pitch;
    - a stateless or object-owned `NativePixelTransfer`.
    X11 updates its native table when the palette changes. SDL3 can either use
    the same native table for an ARGB/RGBA streaming texture or upload indexed
    pixels through a palette expansion step owned by the SDL3 driver.

15. **Move all X11 file-scope state into X11-owned objects.**
    Split `DisplayDeviceX11.cc` into smaller X11-only implementation objects:
    `X11DisplayContext`, `X11Window`, `X11ImageSurface`, `X11PaletteMapper`,
    `X11TextRenderer`, `X11FrameDumper`, and optional `X11Panel`. Move current
    file-scope values such as `display_mit_shm`, `display_on_root`,
    `display_override_redirect`, `private_cmap`, `full_screen`, `window_pos`,
    `xcth_panel`, `xcth_font`, `xcth_display`, `xcth_app_con`, and frame-dump
    counters into those objects. `X11Config` is copied into an immutable
    `X11DisplaySettings` passed at construction/open time.

16. **Untangle the X11 panel from runtime command dispatch.**
    The panel may remain an X11 driver feature, but it must not own Scene,
    `ImageOption`, `RuntimeCommandSink`, `RuntimeCommandTargetRouter`, or
    `RuntimeConfigRegistry` directly. Give it explicit narrow ports:
    `DisplayPanelModel` for read-only palette/selection/metadata snapshots and
    `DisplayPanelEventSink` for user actions. `Application` wires those ports to
    Commands/Input or Runtime Reconfiguration. The display driver only publishes
    panel events; it does not decide or dispatch runtime commands.

17. **Start SDL3 at the new driver seam, not as a fork of X11.**
    After `DisplayDriverFactory`, `DisplayPresentation`, `DisplayTextMetrics`,
    and `NativePixelTransfer` are in place, add `DisplayDriverSDL3.cc/.h` as a
    new implementation of the same driver contract. The first SDL3 driver
    should support:
    - SDL video initialization and shutdown;
    - window creation from common `DisplayConfig` geometry;
    - event polling into `InputEventSink&` with key and close events;
    - resize/expose tracking through `DisplayEventStats`;
    - indexed-frame presentation through a streaming texture;
    - viewport scaling and border clear using the common viewport policy;
    - overlay text drawn with the shared bitmap font.
    Defer X11 panel parity for SDL3; the initial SDL3 driver should prove the
    common presentation contract, not replicate every X11 widget feature.

18. **Keep X11 and SDL3 coexisting behind runtime selection.**
    When both drivers are compiled, one executable should be able to select
    either driver via `display.driver`. Driver-specific config remains typed:
    `X11Config` is used only by the X11 factory, and future `SDL3Config` is used
    only by the SDL3 factory. Shared values stay in `DisplayConfig`. Tests
    should cover X11-only, SDL3-only, both-drivers, `auto`, explicit-supported,
    and explicit-unsupported selection behavior without opening real windows.

19. **Document and test lifecycle order.**
    `Application` construction and shutdown must make these lifetimes true:
    - process services and configuration outlive `DisplaySystem`;
    - Commands/Input ports outlive display event polling;
    - Interface overlay source and error-message source outlive presentation;
    - Frame Generator storage outlives the `IndexedFrame` borrowed by
      `DisplaySystem::present(...)`;
    - Display closes before Frame Generator storage is destroyed;
    - driver platform resources close before backend/device members are
      destroyed;
    - no display alias remains after `DisplaySystem::close()`.

20. **Completion gate.**
    Run the normal build/test/check sequence and the new boundary tests. The
    Display refactor is not complete until production `src/` has no display
    globals or X11 globals reachable from normal runtime code, no display reads
    of `CthughaBuffer::current`, no Interface writes through `DisplayDevice`,
    and both X11 and SDL3 can be selected through the common driver registry
    when compiled.

## Recommended Next Module

The next high-value module is Frame Generator / video filterchain.

Scene's runtime boundary is explicit now. The remaining Scene-facing legacy code
is the well-named `LegacyScene*` adapter layer, which exists because production
visual catalogs and selections are still backed by global `EffectControl`
objects, `VideoDirector`, and `CthughaBuffer`. Deglobalizing frame generation and
native visual catalogs should make those adapters fall away naturally.

Recommended Frame/Visual order:

1. Introduce owned `FrameStore`/`FrameGeometry` while keeping behavior compatible
   with `CthughaBuffer`.
2. Split `VideoDirector` so frame composition/filterchain ownership moves toward
   `FrameComposer` and Scene receives only cue/transition-facing ports.
3. Move frame/window sizing for Scene and Audio from `CthughaBuffer::buffer` to
   explicit geometry values.
4. Introduce native visual catalogs/selections for flames, waves, translations,
   palettes, images, border, and flashlight.
5. Replace `createLegacySceneVisualCatalogFactory(...)` in `Application`.
6. Delete `LegacyScene*` once no production path needs legacy visual
   `EffectControl&` adapters.

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
  geometry, Audio, Frame Generator, presentation, and teardown.

## SDL3 macOS Buildout And Test Plan

This section describes how to build the SDL3 Display driver and prove it works
on macOS without weakening the explicit-dependency goal. It is intentionally
more operational than the module plan above. The expected outcome is a display
driver that can coexist with X11 in the same source tree, can be compiled on
macOS without X11/XQuartz, can be selected at runtime when compiled, and can be
tested with fast headless unit tests plus a small number of explicit windowed
macOS smoke tests.

Reference assumptions to re-check at implementation time:

- SDL3 headers are included as `<SDL3/SDL.h>`.
- SDL3 CMake packages provide an imported target named `SDL3::SDL3`.
- The SDL3 video/render flow is based on `SDL_Init(SDL_INIT_VIDEO)`,
  `SDL_CreateWindow`, `SDL_CreateRenderer`, `SDL_CreateTexture`,
  `SDL_LockTexture`/`SDL_UnlockTexture`, `SDL_RenderTexture`,
  `SDL_RenderPresent`, and `SDL_PollEvent`.
- SDL3 event names use the `SDL_EVENT_*` form, including key, quit, window
  resize, window pixel-size, expose, and close-request events.
- macOS windowed tests must run from a GUI login session and on the main thread.
  They should not be part of the default noninteractive test run.

### macOS Prerequisites

Use Homebrew for the first macOS path because it supplies an SDL3 package with a
CMake config file and keeps the developer workflow short. The initial
verification matrix should cover both Apple Silicon and Intel macOS if possible,
but one architecture is enough for the first local proof.

Install tools:

```sh
xcode-select --install
brew update
brew install cmake ninja sdl3
```

Use a dedicated build directory so X11 and SDL3 experiments never overwrite the
known X11 build:

```sh
cmake -S . -B build-sdl3-macos -G Ninja \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCTH_BUILD_BENCHMARKS=OFF \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
cmake --build build-sdl3-macos
ctest --test-dir build-sdl3-macos --output-on-failure
```

If CMake cannot locate SDL3 through `brew --prefix sdl3`, retry with the broader
Homebrew prefix:

```sh
cmake -S . -B build-sdl3-macos -G Ninja \
  -DCTH_BUILD_X11=OFF \
  -DCTH_BUILD_SDL3=ON \
  -DCTH_BUILD_TESTS=ON \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"
```

Do not require an app bundle for the first driver. A command-line executable run
from Terminal is sufficient for development. App bundle, Info.plist, signing,
and notarization concerns belong to a later packaging pass after the driver
contract is stable.

### Build-System Shape

The build must support three useful display configurations:

1. `CTH_BUILD_X11=ON`, `CTH_BUILD_SDL3=OFF`: current Linux/X11 behavior.
2. `CTH_BUILD_X11=OFF`, `CTH_BUILD_SDL3=ON`: macOS SDL3 behavior with no X11
   requirement.
3. `CTH_BUILD_X11=ON`, `CTH_BUILD_SDL3=ON`: one executable with runtime driver
   selection, useful on systems where both frontends are installed.

Add these CMake controls:

```cmake
option(CTH_BUILD_SDL3 "Build the SDL3 display driver" OFF)
```

When `CTH_BUILD_SDL3` is enabled:

- call `find_package(SDL3 CONFIG REQUIRED)`;
- add SDL3 driver sources to the display executable;
- add a compile definition such as `CTH_SDL3`;
- link `SDL3::SDL3`;
- add the SDL3 include path only through the imported target, not manually;
- keep SDL3 source files out of X11-only builds.

The eventual target should be a display-neutral executable such as `cthugha`
rather than an X11-named target. During migration, keep `xcthugha` working until
the runtime registry exists. Once both drivers share the same display contract,
rename or add the neutral target and optionally keep `xcthugha` as a compatibility
alias only if the packaging path needs it.

The build should not make X11 mandatory on macOS. `CTH_BUILD_X11=OFF` must skip
all `find_package(X11 ...)`, X11 include directories, X11 libraries, and
`CTH_XWIN` compile definitions. This is the key portability gate.

### Configuration Shape

Add a startup display-driver selection that is independent from driver-specific
settings:

```c++
enum DisplayDriverId {
  DisplayDriverAuto,
  DisplayDriverX11,
  DisplayDriverSDL3
};

struct DisplayConfig {
  DisplayDriverId driver;
  ...
};
```

Parse:

- ini key: `display.driver`;
- command-line option: `--display-driver DRIVER`;
- allowed values: `auto`, `x11`, `sdl3`.

Selection rules:

- `auto` chooses from compiled and registered factories using deterministic
  priority. Preserve existing behavior by preferring X11 when X11 is compiled,
  otherwise choose SDL3.
- `x11` requires an X11 factory to be registered.
- `sdl3` requires an SDL3 factory to be registered.
- an unavailable explicit driver is a configuration error, not a silent fallback.
- the chosen driver id should be visible in debug/startup logs.

Keep driver-specific config typed:

- `X11Config` is only consumed by the X11 factory.
- Add `SDL3Config` only for settings SDL3 genuinely owns.
- Shared geometry, zoom, FPS overlay, presentation choice, and frame-size values
  stay in `DisplayConfig`.

Initial `SDL3Config` should be small:

```c++
struct SDL3Config {
  int highPixelDensityEnabled;
  int resizableWindowEnabled;
  std::string rendererName;
  std::string frameDumpDirectory;
  int frameDumpLimit;
  int frameDumpEvery;
};
```

Do not add SDL3 settings for X11-only concepts such as root-window drawing,
private colormaps, MIT-SHM, X11 fonts, override-redirect, or Athena panel
widgets.

### Driver Contracts

Introduce the driver seam before writing SDL3-specific code. The seam should be
testable without opening a real SDL window.

Core construction types:

```c++
class DisplayOpenRequest {
public:
  DisplayConfig display;
  X11Config* x11;       // null unless X11 is compiled and selected
  SDL3Config* sdl3;     // null unless SDL3 is compiled and selected
  PixelSize initialFrameSize;
  LogSink& log;
};

class DisplayDriver {
public:
  virtual ~DisplayDriver() { }
  virtual int open(const DisplayOpenRequest& request) = 0;
  virtual DisplayEventStats processEvents(InputEventSink& input,
                                          DisplayLifecycleEventSink& lifecycle) = 0;
  virtual PixelSize outputSize() const = 0;
  virtual void present(const DisplayPresentation& presentation) = 0;
  virtual void close() = 0;
};

class DisplayDriverFactory {
public:
  virtual ~DisplayDriverFactory() { }
  virtual DisplayDriverId id() const = 0;
  virtual const char* name() const = 0;
  virtual std::unique_ptr<DisplayDriver> create() = 0;
};
```

`DisplayLifecycleEventSink` is important. Window close and app quit events are
lifecycle events, not keyboard input. SDL3 should not translate
`SDL_EVENT_QUIT` into a fake key and should not dispatch `RuntimeCommand`
objects from inside the driver. It should publish a close request through a
narrow lifecycle sink or through a typed field in `DisplayEventStats`, and
`Application` should route that to `RuntimeShutdown`.

The driver contract must be one-way:

- Display receives raw input and lifecycle events from the platform.
- Display receives completed indexed frames from Frame Generator.
- Display receives overlay commands from Interface.
- Display presents to the platform.
- Display never pulls scene state, frame storage, runtime commands, audio state,
  or interface state through globals.

### SDL3 Driver Object Model

Implement SDL3 through small owned objects rather than one large file-scope
blob. The first pass can keep them in one `.cc` file if needed, but the
ownership model should be clear.

Recommended objects:

- `Sdl3VideoContext`: owns SDL video initialization and final shutdown.
- `Sdl3Window`: owns `SDL_Window*`, window flags, logical window size, and
  high-DPI/pixel-size tracking.
- `Sdl3Renderer`: owns `SDL_Renderer*`, render output size queries, clear,
  texture draw, overlay draw, and present.
- `Sdl3StreamingTexture`: owns `SDL_Texture*`, texture size, pixel format, pitch
  negotiation, lock/unlock, and resize-on-frame-size-change behavior.
- `Sdl3PaletteExpander`: owns the 256-entry palette expansion table and converts
  indexed pixels to the texture pixel format.
- `Sdl3OverlayRenderer`: uses the existing `BitmapFont`/`dosVga9x14Font()` to
  draw overlay commands into the SDL renderer or into the streaming texture.
- `Sdl3EventPump`: maps SDL events into `InputEventSink`,
  `DisplayLifecycleEventSink`, and `DisplayEventStats`.
- `Sdl3FrameDumper`: optional diagnostic that writes the expanded frame or final
  pre-overlay frame to a portable image format for tests.

No SDL3 file should define mutable process-wide state. Every pointer returned by
SDL is owned by exactly one object and destroyed in that object's destructor or
`close()` method. A failed open should release already-created SDL resources
before returning failure.

### SDL3 Open And Close Lifecycle

Open order:

1. `DisplaySystem::open(...)` selects `DisplayDriverSDL3Factory`.
2. `DisplayDriverSDL3` constructs empty owned members.
3. `Sdl3VideoContext::open()` initializes SDL video.
4. `Sdl3Window::open()` creates the window from common `DisplayConfig`.
5. `Sdl3Renderer::open()` creates the renderer from the window.
6. `Sdl3Renderer` records the initial render output size.
7. `Sdl3OverlayRenderer` receives `DisplayTextMetrics` and the shared bitmap
   font.
8. The driver reports open success only after window, renderer, and initial
   output-size query all succeed.

Close order:

1. Stop using the current presentation frame.
2. Destroy the streaming texture.
3. Destroy renderer-owned overlay resources.
4. Destroy the renderer.
5. Destroy the window.
6. Quit the SDL video subsystem if this driver initialized it.
7. Clear all owned pointers and make `close()` idempotent.

Main-thread rule:

- SDL initialization, event polling, window operations, renderer operations, and
  texture updates should all happen on the main application thread.
- Do not add a display thread for SDL3.
- Do not write tests that open SDL windows on worker threads.

### SDL3 Window And Output Geometry

Use common display geometry for window creation:

- `DisplayConfig.displayWidth/displayHeight` when custom display size is set;
- otherwise the selected display mode size;
- initial frame size from Frame Generator only as a minimum texture/frame
  reference, not as a mutable global.

Window flags:

- use a resizable window by default unless configuration says otherwise;
- use high-pixel-density mode when enabled;
- use fullscreen only when the common display config requests fullscreen through
  a shared setting added for both drivers;
- do not implement X11 root-window behavior in SDL3.

Separate these sizes:

- logical window size: what the user asked for and what window events report;
- render output size: actual drawable pixels, especially important on Retina;
- source frame size: the indexed frame from Frame Generator;
- composed indexed display frame size: output of `PresentationComposer`;
- viewport destination: destination rectangle for rendering into output pixels.

`DisplayDriverSDL3::outputSize()` should return render output pixels, not
logical window points. On macOS Retina displays this distinction prevents blurry
or incorrectly scaled output.

On resize and pixel-size events:

- increment `DisplayEventStats.resizeEvents`;
- update cached render output size;
- mark the next presentation as needing a border/background clear;
- do not resize Frame Generator storage;
- do not mutate DisplayConfig.

### SDL3 Texture And Pixel Transfer

The first SDL3 renderer should use a streaming texture because it maps cleanly
onto Cthugha's "indexed frame plus palette" model.

Texture policy:

- create one streaming texture whose size matches the composed
  `IndexedDisplayFrame`;
- recreate it when composed frame width or height changes;
- use a predictable 32-bit format such as RGBA8888 or ARGB8888;
- set nearest-neighbor scaling so indexed artwork stays crisp;
- keep all palette expansion state in `Sdl3PaletteExpander` or the common
  `NativePixelTransfer`, not in globals.

Present path:

1. Receive `DisplayPresentation` from `DisplaySystem`.
2. Validate the composed indexed frame.
3. Ensure streaming texture size matches the frame.
4. Update the native palette table when `FramePalette` is dirty or when text
   overlay palette rules require it.
5. Lock the streaming texture.
6. Expand indexed source pixels into the locked texture using the locked pitch.
7. Unlock the texture.
8. Clear the renderer to black.
9. Draw the texture into the viewport destination rectangle.
10. Draw overlay commands.
11. Present the renderer.

Do not use X11's `needsFullCopy` semantics as an SDL3 control path. SDL3 can
clear the renderer every frame and render the current texture into the current
viewport. Keep the flags in `DisplayPresentation` for compatibility and tests,
but treat them as hints rather than a requirement for partial native copies.

### SDL3 Palette Behavior

The current X11 display has special palette behavior when text is on screen.
SDL3 should preserve visible behavior without preserving X11's colormap
mechanics.

Rules:

- the indexed visual frame remains unchanged;
- palette expansion applies text-darkening or text-reserved colors only in the
  presentation step;
- text overlay colors use Display-owned constants or `DisplayTextPalette`;
- `FramePalette::paletteDirty()` is acknowledged only by the display backend
  that consumed it;
- SDL3 should not write `bitmap_colors*` or rely on `draw_mode`.

For true-color SDL3 output, implement text dimming by choosing overlay/text
colors and optional frame darkening in the expanded RGBA pixels. Do not simulate
X11 PseudoColor cell allocation.

### SDL3 Overlay Rendering

The first SDL3 overlay renderer should not use SDL_ttf. Use the existing
bitmap font to avoid adding another dependency.

Overlay pipeline:

1. Interface produces `OverlayCommands` through `InterfaceOverlaySource`.
2. Display computes `OverlayLayout` from `DisplayTextMetrics` and render output
   size.
3. SDL3 draws the frame texture.
4. SDL3 draws each overlay text command using the bitmap font.
5. SDL3 presents the renderer.

Text coordinates:

- `OverlayTextCommand.y` keeps the existing line-based behavior;
- positive `y` counts down from the top;
- negative `y` counts up from the bottom;
- justification `l`, `c`, and `r` are applied using `OverlayLayout.columns`;
- clipping happens inside `Sdl3OverlayRenderer`.

Text colors:

- `TEXT_COLOR_NORMAL`, `TEXT_COLOR_ERROR`, and `TEXT_COLOR_HIGHLIGHT` should
  move to a display-owned enum or value object;
- SDL3 maps those values to RGBA colors;
- palette darkening should be a display presentation effect, not an Interface
  side effect.

### SDL3 Event Mapping

Create pure mapping helpers before touching the real event loop. The helpers
make event behavior testable without opening a window.

Event categories:

- quit/close request: publish to `DisplayLifecycleEventSink`;
- key up: publish raw key text plus shift state to `InputEventSink`;
- window resize/pixel-size changed: update output size and stats;
- expose: mark a redraw/full-clear request and stats;
- all other events: ignore unless a future SDL3 feature needs them.

Keyboard mapping must match the current `KeyTranslator` expectations:

- letters and digits should produce their one-character text;
- shifted digits should preserve the existing shifted-number behavior through
  the `shifted` flag;
- function keys should produce `F1` through `F24`;
- arrow keys should produce `Up`, `Down`, `Left`, `Right`;
- page up/down should produce `Prior` and `Next` to match the current X11 names;
- return should produce `Return`;
- backspace should produce `BackSpace`;
- delete should produce `Delete`;
- home/end should produce `Home` and `End`;
- keypad digits should produce `KP_0` through `KP_9` or direct digits, whichever
  the existing translator expects after tests confirm behavior;
- escape should produce the same raw text currently accepted by `InputQueue`.

Do not let SDL3-specific key names leak throughout the application. Keep them in
`Sdl3KeyMapper`, and test that mapper directly.

### macOS Test Layers

Use four layers of tests. The first three run in normal `ctest`; the fourth is
explicitly opt-in because it opens real windows.

1. **Source-boundary tests.**
   These check that SDL3 is behind the driver seam and that Display remains
   explicit:
   - `Application.cc` does not include SDL headers;
   - only SDL3 driver files include `<SDL3/...>`;
   - SDL3 files do not include X11 headers;
   - X11 files do not include SDL3 headers;
   - Interface files do not include display backend headers;
   - Display driver files do not include Frame Generator implementation headers.

2. **Pure unit tests.**
   These do not call SDL:
   - driver registry selection for `auto`, explicit `x11`, explicit `sdl3`,
     unavailable explicit driver, and no available drivers;
   - `DisplayConfig` parsing for `display.driver`;
   - `SDL3Config` parsing for any SDL3-specific keys;
   - SDL3 key-name mapper;
   - viewport selection for Retina render-output sizes;
   - RGBA palette expansion for representative palette entries;
   - texture resize decision logic;
   - overlay layout and clipping;
   - lifecycle close-request routing.

3. **SDL API adapter tests with fakes.**
   Wrap SDL calls behind a tiny `Sdl3Api` interface only where it helps
   lifecycle testing. Use a fake implementation to test:
   - open creates video, window, renderer, and texture in order;
   - open failure at each stage releases earlier resources;
   - `close()` is idempotent;
   - texture recreation happens only on geometry changes;
   - present locks, writes, unlocks, clears, renders, overlays, and presents in
     order;
   - event pumping updates stats and sinks correctly.

4. **Windowed macOS smoke tests.**
   These run only when explicitly enabled, for example with
   `CTH_RUN_WINDOW_TESTS=ON` or an environment variable. They require a logged-in
   macOS GUI session. They should not run in default CI unless that CI has a
   real window server.

Windowed smoke targets:

- `sdl3_window_open_smoke`: opens a small SDL3 window, clears it, presents once,
  and exits.
- `sdl3_texture_present_smoke`: presents a deterministic indexed gradient
  through the SDL3 texture path for a fixed number of frames, then exits.
- `sdl3_resize_smoke`: opens a resizable window, programmatically requests or
  simulates resize where SDL allows it, and verifies output-size bookkeeping.
- `sdl3_event_smoke`: opens a window and exits when the close event is observed.

The smoke binaries should be tiny and isolated from audio and full scene setup
where possible. If the full application is used for a smoke run, pass options
that avoid macOS audio-device dependency:

```sh
./build-sdl3-macos/src/cthugha \
  --display-driver=sdl3 \
  --no-sound \
  --silent \
  --disp-mode=320x200 \
  --max-fps=30 \
  --show-fps \
  --no-save
```

If the executable is still named `xcthugha` during migration, use that name
temporarily but do not encode it into new tests as the permanent SDL3 target.

### macOS Manual Verification Matrix

After unit tests pass, run a short manual verification pass on macOS:

1. **Startup and shutdown.**
   Launch SDL3, verify a window appears, then close the window. Confirm the app
   exits through the lifecycle sink and logs a clean shutdown.

2. **Driver selection.**
   Run `--display-driver=sdl3` and confirm SDL3 is selected. Run
   `--display-driver=x11` in an SDL3-only build and confirm startup reports a
   configuration error instead of falling back silently. Run
   `--display-driver=auto` and confirm the selected driver matches the compiled
   registry priority.

3. **Retina output.**
   On a Retina display, log logical window size and render output size. Confirm
   viewport math uses render output pixels and that the image is sharp with
   nearest-neighbor scaling.

4. **Resize.**
   Resize the window smaller, larger, and across aspect ratios. Confirm the
   image remains centered, letterbox borders clear to black, and no frame
   storage resize is triggered by display resize.

5. **Keyboard.**
   Verify letters, shifted letters, digits, shifted digits, arrows, page
   up/down, home/end, return, backspace, delete, escape, function keys, and
   keypad digits. Confirm behavior matches X11 keymap behavior.

6. **Overlay.**
   Toggle interface panels and FPS overlay. Confirm normal, highlighted, and
   error text are legible, aligned, clipped, and removed cleanly on subsequent
   frames.

7. **Frame presentation.**
   Run several display modes and zoom values. Confirm source-sized,
   double-sized, and presentation effects display correctly.

8. **Fullscreen.**
   If fullscreen support is added in the first SDL3 pass, verify entering and
   exiting fullscreen. If not, explicitly document fullscreen as deferred and
   ensure the config parser rejects or ignores it with a clear diagnostic.

9. **Failure behavior.**
   Temporarily request an invalid renderer name or impossible texture size in a
   test build. Confirm startup failure releases SDL resources and reports the
   reason.

10. **No X11 dependency.**
    Configure with `CTH_BUILD_X11=OFF`, uninstall or hide XQuartz if present,
    and verify the SDL3 build still configures, builds, and runs.

### Debug And Diagnostics

Add debug logs that make SDL3 problems diagnosable without a debugger:

- selected display driver;
- SDL version/build information if available through SDL APIs;
- window logical size;
- render output size;
- texture format and size;
- palette expansion format;
- renderer name;
- high-pixel-density setting;
- each resize/pixel-size event;
- close-request events;
- present failures.

Add an SDL3 frame dump option mirroring the X11 frame-dump idea, but keep it
driver-local:

- `sdl3.frame_dump_directory`;
- `sdl3.frame_dump_limit`;
- `sdl3.frame_dump_every`.

Dump either the expanded texture pixels before overlay or the final composited
software buffer if the overlay renderer draws into software. Do not require
macOS screenshots for automated pixel verification.

### Incremental Implementation Order

1. Add `CTH_BUILD_SDL3` CMake option, SDL3 package lookup, and an empty
   compiled-in SDL3 source file behind the option.
2. Add `DisplayDriverId`, parser support, config tests, and help text.
3. Add `DisplayDriverFactory`, `DisplayDriverRegistry`, and fake-factory tests.
4. Refactor `Application` enough that driver selection lives in the Display
   registry rather than in X11-specific calls.
5. Add `DisplayLifecycleEventSink` or a close-request field in
   `DisplayEventStats`.
6. Add pure SDL3 key mapper and tests.
7. Add pure SDL3 palette expansion and overlay layout tests.
8. Implement `Sdl3VideoContext`, `Sdl3Window`, and `Sdl3Renderer` with fake API
   lifecycle tests.
9. Implement the streaming texture present path with a deterministic unit test
   over fake locked texture memory.
10. Implement the real SDL3 event pump.
11. Add a small window-open smoke binary gated behind an opt-in CMake option or
    environment variable.
12. Run the macOS SDL3-only build with `CTH_BUILD_X11=OFF`.
13. Run the full application with `--display-driver=sdl3 --no-sound --silent`.
14. Add coexistence tests for builds where both X11 and SDL3 are compiled.
15. Only after SDL3 is stable, decide whether the default `auto` priority should
    remain X11-first or become platform-specific.

### Acceptance Criteria

The SDL3 macOS buildout is complete when:

- `cmake -S . -B build-sdl3-macos -G Ninja -DCTH_BUILD_X11=OFF
  -DCTH_BUILD_SDL3=ON -DCTH_BUILD_TESTS=ON` configures on macOS with Homebrew
  SDL3;
- `cmake --build build-sdl3-macos` succeeds;
- default `ctest` passes without opening windows;
- opt-in SDL3 window smoke tests pass from a macOS GUI session;
- the full application opens and presents frames with
  `--display-driver=sdl3 --no-sound --silent`;
- explicit unsupported driver selection fails with a configuration diagnostic;
- SDL3 source files contain the only SDL includes;
- X11 and SDL3 drivers can coexist behind the same registry when both are
  compiled;
- no SDL3 path reads or writes display globals, X11 globals, Interface globals,
  or `CthughaBuffer::current`;
- Display still receives all dependencies through constructors, open requests,
  per-frame method parameters, or `DisplaySystem`-owned objects.
