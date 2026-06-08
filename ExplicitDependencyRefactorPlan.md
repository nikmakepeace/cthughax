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

The remaining Scene-facing debt is intentionally quarantined in legacy
list/interface compatibility:

- `Application` creates the native
  `createSceneVisualCatalogServiceFactory(...)` from native
  `SceneVisualSelectionSeeds`; startup names are applied through
  `SceneRuntime::applyStartupConfig(...)`.
- `SceneWaveObjectCatalogLoader`, `SceneImageCatalogLoader`, and
  `ScenePaletteCatalogLoader` load directly into native Scene catalogs instead
  of copying data from compatibility visual option lists.
- Legacy visual option lists still exist for non-Scene Display/UI paths until
  those paths read Scene visual choices directly.

#### Services Needed

- Owned visual catalogs for flame, wave, table, object, translation, palette,
  image, border, and flashlight choices, including allowed-choice metadata.
- `FrameGeometry` from Frame Generator so Scene and visual catalogs do not depend
  on `VideoDirector`/`CthughaBuffer` as geometry providers.
- Boundary tests that keep native visual loaders from depending on
  compatibility `EffectControl`/`ImageOption` catalogs.

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

1. Keep visual compatibility isolated to named loader/UI surfaces while the
   visual/filterchain runtime is still legacy-backed.
2. Deglobalize Frame Generator: introduce owned frame storage/geometry and split
   `VideoDirector`/filterchain responsibilities away from `CthughaBuffer`
   aliases.
3. Introduce native visual catalogs and selection storage for flames, waves,
   translations, palettes, images, border, flashlight, and related metadata.
4. Delete remaining compatibility loaders and boundary exceptions once no
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
- `FrameRenderContext` and `ScreenRenderContext` carry audio products into
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

The module already has useful per-frame handoffs. `FrameGeneratorContext`
carries borrowed audio buffers, an immutable `AudioAnalysisSnapshot`, frame
timing, frame budget, and a `SceneSnapshot` into frame filters. The filterchain
can publish an `IndexedFrame` for
`CthughaDisplay::present(...)`, and filter-local state such as
`FlameLookupTables`, `WaveState`, and `WaveLookupTables` is already object-owned.

The remaining compatibility surfaces are:

- Legacy visual option/list surfaces still used by non-Scene Display/UI paths;
- Display compatibility aliases and `display.cc` renderer statics, which are
  tracked as Display follow-up because generated frames are already handed to
  Display explicitly.

#### Services Needed

- `FrameGeneratorRuntime`: Application-owned module root. Owns frame storage,
  filterchain/pipeline state, scene binding, frame-generation settings, and
  generator diagnostics.
- `FrameGeometry`: stable logical visible frame dimensions and derived values
  (`width`, `height`, `maxDimension`). It implements or adapts to
  `SceneGeometry` but is not a buffer and does not describe row storage.
- `FrameStorageLayout`: FrameStore-owned storage policy for visible size, row
  pitch/stride, top/bottom hidden rows, allocation size, and visible-start
  offsets.
- `FrameStore`: owned indexed active/passive storage, storage layout,
  active/passive selection, clear, resize, and swap operations.
- `FrameGeneratorContext`: per-frame borrowed inputs: `SceneSnapshot`,
  `AudioFrame`, raw/processed audio buffers, `AudioAnalysisSnapshot`, timing,
  and frame-rate budget values.
- `FrameGeneratorSceneBinding`: observes a `Scene` for changes and one-shot
  image/text cues. It queues generator work but does not own scene selection
  policy.
- `FrameGeneratorPipeline` / `FrameComposer`: owns the filterchain and
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
  int maxDimension() const;
};

class FrameStorageLayout {
public:
  PixelSize visibleSize() const;
  int width() const;
  int height() const;
  int pitch() const;
  int topHiddenRows() const;
  int bottomHiddenRows() const;
  int allocationByteCount() const;
  int visibleOffset(int x, int y) const;
  int visibleLinearOffset(int linearOffset) const;
};

class FrameStore {
public:
  void resize(const FrameGeometry& geometry);
  void resize(const FrameStorageLayout& layout);
  const FrameGeometry& geometry() const;
  const FrameStorageLayout& layout() const;
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

The Frame Generator migration plan is a compatibility-surface audit. Each item
names the temporary surface, the job it performs, the concrete code changes
that retire it, and the gate that must be true before the item can be erased
from this plan.

1. **Retire the `CthughaBuffer`/`VideoDirector` global storage path. Status:
   complete.**
   Compatibility surface: the old process-global `CthughaBuffer` storage and
   `VideoDirector` entry points.

   Purpose: keep active/passive indexed buffers, "current frame" lookup, scene
   observation, image/text cues, palette transitions, and filterchain execution
   available while callers were moved to owned Frame Generator objects.

   Completion work:
   - make `FrameGeneratorRuntime` own the frame-generation lifetime;
   - make `FrameStore` own active/passive indexed storage, clears, resize, and
     swaps;
   - split visible dimensions from storage details with `FrameGeometry` and
     `FrameStorageLayout`;
   - move generator-side scene observation and cue consumption into
     `FrameGeneratorSceneBinding`;
   - move palette/image transition state into `FrameTransitionController`;
   - execute rendering through `FrameGeneratorPipeline` using explicit inputs;
   - pass the returned `IndexedFrame` from `Application` to Display;
   - remove Display fallback reads from `CthughaBuffer::current` or
     `CthughaBuffer::buffer`.

   Completion gate: `VideoDirector` is deleted; static `CthughaBuffer` aliases
   are gone; production frame delivery uses the `IndexedFrame` returned by
   `FrameGeneratorRuntime::render(...)`; Display never reaches back into
   generator storage; and boundary tests reject global buffer aliases in the
   Frame Generator path.

2. **Rename or port legacy `VideoFilterchain`, `VideoFilters`,
   `VideoFrameContext`, and `VideoFrameBudget` APIs. Status: complete.**
   Compatibility surface: the old `Video*` headers, source files, class names,
   helper APIs, CMake references, tests, and benchmarks that still named frame
   generation as "video".

   Purpose: keep call sites compiling while storage, context, budget, and
   pipeline ownership moved from the legacy video/director vocabulary to the
   explicit Frame Generator vocabulary. These surfaces were naming and API
   compatibility only; they were not needed for distinct runtime behavior.

   Completion work:
   - port `VideoFilterchain*` headers, sources, classes, and includes to
     `FrameFilterchain*`;
   - port `VideoFilters*` headers, sources, classes, and includes to
     `FrameFilters*`;
   - replace public `VideoFrameContext` use with `FrameGeneratorContext`;
   - replace public `VideoFrameBudget*` use with `FrameGeneratorFrameBudget*`;
   - update `FrameGeneratorRuntime`, `FrameGeneratorPipeline`, filter code,
     `Application`, tests, and benchmarks to include and call the new names;
   - update CMake source lists and test target names so the build graph no
     longer advertises the old generator names;
   - leave any unavoidable `Video*` name only inside a private legacy adapter,
     with a boundary test documenting why that adapter remains.

   Completion gate: public Frame Generator headers, method signatures, exported
   type names, CMake targets, tests, benchmarks, and production callers all use
   `Frame*`/`FrameGenerator*` names. Any remaining `Video*` symbol must be
   isolated in a private legacy adapter outside the public generator API and
   documented by a boundary test. The current generator path satisfies this:
   it uses `FrameFilterchain*`, `FrameFilters*`, `FrameGeneratorContext`,
   `FrameGeneratorFrameBudget`, `FrameGeneratorPipeline`, and
   `FrameGeneratorRuntime`, and the legacy `Video*` generator files are gone.

3. **Replace generator-side legacy logging macros with explicit `LogSink&`.
   Status: complete.**
   Compatibility surface: direct use of legacy `CTH_*` logging macros from
   generator and filter code.

   Purpose: keep diagnostics working before Frame Generator had an injectable
   diagnostics dependency. The macros reached through the process-level logging
   bridge, so they were a temporary substitute for a real module port.

   Completion work:
   - pass a `LogSink&` into `FrameGeneratorRuntime`;
   - thread diagnostics into pipeline, filterchain, scene-binding,
     filter-frame, and wave-runtime code through constructors or method
     parameters;
   - replace configure/render/filter macro calls with explicit `LogSink`
     operations;
   - keep any process-level logging adapter at the application boundary, not in
     generator sources;
   - add unit tests that inject recording or silent log sinks;
   - add boundary tests rejecting `CTH_DEBUG`, `CTH_INFO`, `CTH_WARN`,
     `CTH_ERROR`, `CTH_TRACE`, and `CTH_LOG_ENABLED` in the generator path.

   Completion gate: every generator diagnostic call flows through an explicit
   `LogSink&` or a narrow object that owns one; the process-level logging bridge
   remains only at application or legacy-module boundaries; unit tests can
   inject recording/silent sinks; and boundary tests block legacy logging macro
   calls from generator sources.

4. **Replace the generator render input compatibility bundle. Status: complete
   for Frame Generator.**
   Compatibility surface: the old broad render-input bundle that mixed raw
   audio, processed wave data, `AudioMetrics`, `AcousticContext`, scene
   snapshot data, timing, display-facing frame context, and frame budget values.

   Purpose: preserve the old render call shape while Audio analysis, Scene
   snapshots, timing, and frame-budget values were split into explicit module
   inputs.

   Completion work:
   - make `FrameGeneratorContext` the only render input bundle accepted by the
     generator;
   - make Audio provide immutable `AudioAnalysisSnapshot` values instead of
     exposing rolling analyzer or acoustic-context state;
   - put borrow-only audio frame pointers, raw/processed audio buffers, scene
     snapshot, timing, and explicit frame-budget values into
     `FrameGeneratorContext`;
   - remove `VideoFrameContext`, `FrameRenderContext`, `AcousticContext`, and
     `AudioAnalyzer.h` from generator headers and sources;
   - keep any remaining context translation in Audio, Scene, Display, or
     application boundary code;
   - add boundary tests blocking the older context types from generator sources.

   Completion gate: `FrameGeneratorRuntime::render(...)` and
   `FrameGeneratorPipeline` accept only `FrameGeneratorContext`; generator
   headers and sources do not include or mention `VideoFrameContext`,
   `FrameRenderContext`, `AcousticContext`, or `AudioAnalyzer`; and any
   translation between older context concepts happens outside Frame Generator.
   `FrameRenderContext` and `AcousticContext` may still exist in Scene, Audio,
   or Display follow-up work, but they are no longer generator render inputs.

5. **Move visual catalogs and selections off global `EffectControl` objects.
   Status: remaining.**
   Compatibility surface: temporary visual `EffectControl` entries still used
   to load, name, lock, select, randomize, or mirror visual choices.

   Purpose: stand in for native visual owners while the old visual system still
   owns file loading, generated catalog construction, current-value lookup,
   randomization, lock/use state, command routing, and persistence. These
   surfaces keep flames, general flame, waves, wave scale, tables, objects,
   translations, palettes, images, border, and flashlight visible to Scene and
   Frame Generator until every domain has a typed owner.

   Completion work:
   - add native catalog owners for every visual domain consumed by Scene or
     Frame Generator: flame, general flame, wave, wave scale, table,
     translation, object, palette, image, border, and flashlight;
   - move file-backed loading and generated catalog construction into those
     owners, so objects, palettes, images, and translations load into owned
     typed entries rather than temporary `EffectChoice` entries;
   - move built-in choice metadata into native catalogs, so availability no
     longer depends on `EffectChoice::inUse()` or `EffectChoiceList`;
   - move current-value lookup into native selections or typed owner ports, so
     Scene never asks an `EffectControl` for the selected payload;
   - move visual randomization and mutation behavior into native services,
     including random palette changes and add-random-palette behavior;
   - move lock/use state into native selections, with command APIs that edit
     typed selections rather than generic controls;
   - move persistence for visual selections and file-backed visual catalog
     entries into Scene/native visual serializers;
   - provide typed APIs consumed by `SceneVisualCatalogs`,
     `SceneVisualSelections`, `FrameGeneratorSceneBinding`, runtime commands,
     boundary tests, and unit tests;
   - remove production Scene and Frame Generator construction-time includes of
     global visual option headers.

   Completion gate: native visual owners supply every catalog, selected
   payload, allowed-choice value, random operation, lock/use value, and
   persisted value used by Scene or Frame Generator; Scene and Frame Generator
   construction no longer need visual `EffectControl&` inputs; and boundary
   tests block global visual `EffectControl` dependencies from those modules.

   Current progress toward that gate:
   - General-flame selection is native.
   - Flame and wave selections point at typed `Flame` and `Wave` catalog items.
   - Flame and wave default availability is native
     `SceneBuiltInChoiceCatalogs` metadata instead of a read through legacy
     `EffectChoice::inUse()`.
   - Built-in flame and wave catalog construction lives in
     `SceneBuiltInChoiceCatalogs`; native `SceneVisualSelectionFactory` builds
     selections from explicit `SceneVisualSelectionSeeds`.
   - File-backed object, translation, palette, and image choice-catalog
     construction lives in `SceneTypedVisualCatalogs`; `Application` supplies
     default native seed values and startup config applies named choices without
     reading global visual `EffectControl` selection values.
   - Translation entries are generated into native `SceneTranslationCatalog`
     ownership, mirrored into the temporary legacy translation option, and used
     directly by Scene translation selections.
   - Wave-object entries load directly into native
     `SceneWaveObjectCatalog` ownership through `SceneWaveObjectCatalogLoader`,
     and Scene object selections use that native catalog without copying from
     the global object `EffectControl`.
   - Image entries load directly into native `SceneImageCatalog` ownership
     through `SceneImageCatalogLoader`, and Scene image selections use that
     native catalog without copying from the temporary legacy `ImageOption`.
   - `IndexedImage` and `ImageLoadTarget` live behind the narrow
     `IndexedImage.h` payload header. Frame Generator and native Scene image
     catalog code include that header instead of the legacy `Image.h`
     compatibility surface, which still owns `ImageEntry` and `ImageOption`.
   - The temporary legacy `ImageOption` is owned by `Application` for loading,
     Display/interface compatibility, and legacy adapter wiring; public Frame
     Generator APIs no longer expose `ImageOption` or include `Image.h`.
   - Palette entries load directly into native `ScenePaletteCatalog` ownership
     through `ScenePaletteCatalogLoader`, and initial Scene palette selections
     use that native catalog without copying from the global `palette` option.
     Random/add-random palette mutation now uses `ScenePaletteRandomizer`,
     which appends or replaces Scene-owned palette entries and persists
     generated random.N files without reading the global `palette` option.
   - Scene visual settings construction, startup choice application, typed
     selection mutation, and random-palette mutation now live in
     `SceneVisualCatalogService` instead of a `LegacySceneVisualCatalogs`
     implementation. `LegacyScenePaletteRandomizer` is deleted; the separate
     old `PaletteEntry` static random-palette helpers remain only for the legacy
     global palette path and its persistence coverage.
   - `SceneVisualCatalogServiceFactory` now owns the native visual catalog
     service construction, and `LegacySceneVisualCatalogFactory` is deleted.
   - `LegacySceneSelectionFactory` is deleted. `SceneVisualSelectionFactory`
     owns native selection construction from explicit seeds, and
     `SceneVisualSelectionFactoryTest` covers direct native startup seeding
     without constructing visual `EffectControl` globals.
   - `LegacyGlobalSceneSelectionFactory` is deleted. `Application` builds
     default `SceneVisualSelectionSeeds` directly, so Scene runtime construction
     no longer reads current values or locks from global visual controls.
   - Wave-scale, table, border, and flashlight choice metadata is built by
     `SceneBuiltInChoiceCatalogs` instead of borrowed from `EffectChoiceList`.
   - Border and flashlight legacy option globals live behind narrow
     `BorderOption` and `FlashlightOption` compatibility headers, while frame
     filters use the separate renderer ports instead of the global option
     headers.
   - Flame, wave, and translation legacy option globals live behind
     `FlameOptions`, `WaveOptions`, and `TranslationOption` compatibility
     headers. Legacy list/interface and loading paths use those headers instead
     of the broader generation/renderer umbrellas.
   - `Application` creates the native visual catalog service factory and
     `SceneRuntime` after built-in/file-backed visual catalog loading, so Scene
     catalogs see loaded object, translation, palette, and image entries.
   - Global visual headers are limited to Application startup/loading,
     list/interface compatibility, and legacy option implementation files.
     Frame Generator files use Scene ports plus narrow border/flashlight
     renderer ports instead of including global visual option headers.
   - `Application` translates startup `Config` into
     `FrameGeneratorRuntimeConfig` and `SilenceMessageConfig` before
     configuring Frame Generator services. `FrameGeometry`,
     `FrameTransitionController`, `FrameGeneratorRuntime`, `SilenceMessage`,
     and `QotdMessagesProvider` no longer include `Configuration.h` or expose
     `DisplayConfig`, `SceneTransitionPolicy`, or `MessagesConfig`.
   - F2/list interfaces for Scene visual choices are registered from native
     `RuntimeSceneTarget` selections only. The list interface no longer carries
     visual `EffectControl` fallbacks or the legacy visual option headers;
     display-only `EffectControl` list support remains scoped to the display
     option list.
   - X11 panel Scene menus are registered from native `RuntimeSceneTarget`
     selections only. Panel display menus still use display-owned
     `EffectControl` state, while Scene menu choices and activation route
     through native Scene selections and runtime Scene commands.
   - Keymap actions for Scene visual choices emit typed `RuntimeCommand`
     Scene changes, and list/config UI elements pass `RuntimeSceneTarget`
     values through `CommandContext` instead of routing visual choices through
     generic `EffectControl` commands.
   - Runtime config display text reads Scene visual names from
     `RuntimeConfigRegistry` snapshots populated by `SceneSerializer`. UI
     fallbacks for Scene rows come from native `SceneVisualSelections`; the
     display-only `EffectControl` fallback remains scoped to display options.
   - `LegacySceneSelectionAdapters`, `LegacySceneSelectionFactory`,
     `LegacyScenePaletteRandomizer`, `LegacySceneVisualCatalogFactory`,
     `LegacyGlobalSceneSelectionFactory`, and the other `LegacyScene*` bridge
     sources are deleted from production and CMake. Boundary tests assert their
     absence instead of allowing temporary bridge names.

7. **Finish the separate Display cleanup outside Frame Generator. Status:
   related remaining.**
   Compatibility surface: Display's legacy global aliases and renderer statics:
   `cthughaDisplay`, `displayDevice`, `displayBackend`, `displayRuntime`, plus
   presentation, overlay, panel, event, and native pixel-transfer state still
   owned through globals or file-scope/function-local statics.

   Purpose: keep older presentation and interface code running while Display is
   split into explicit module-owned presentation, event, overlay, and renderer
   ports. This item is listed in the Frame Generator section only because
   Display previously read generator storage; it should remain outside the
   Frame Generator dependency path now that frames are handed to Display
   explicitly.

   Completion work:
   - introduce or finish a `DisplaySystem` or presentation root that owns the
     backend, device, facade/coordinator, presentation settings, overlay
     rendering, event collection, and close order;
   - remove normal runtime reads/writes of `cthughaDisplay`, `displayDevice`,
     `displayBackend`, and `displayRuntime`;
   - pass explicit Display ports to callers instead of publishing aliases;
   - move `display.cc` renderer state such as animation counters, height-field
     rotation, scratch buffers, presentation effects, and native pixel-transfer
     state into Display-owned runtime objects;
   - keep frame presentation one-way: `Application` passes an `IndexedFrame`
     plus presentation context/overlays into Display, and Display never asks
     Frame Generator or `CthughaBuffer` for current pixels;
   - route raw display events to Commands/Input through an `InputEventSink` or
     input queue, not through command dispatch or option mutation inside
     Display;
   - give Interface an overlay source/sink handoff so overlay rendering does
     not print through display globals;
   - update X11/SDL/device tests and source-boundary tests around the new
     Display module ports.

   Completion gate: production runtime code has no display aliases or globals;
   Display renderer state is object-owned; Display never reads Frame Generator
   storage; Display never dispatches commands through global state; and Frame
   Generator boundary tests continue to prove Display cleanup is outside the
   generator dependency path.

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
               const FrameRenderContext& context,
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
   produces a `FrameTiming`/`FrameRenderContext` value. Display receives that
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
    - explicit `FrameTiming`/`FrameRenderContext` supplied per frame;
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

The next high-value module is Frame Generator / frame filterchain.

Scene's runtime boundary is explicit now. The remaining Scene-facing
compatibility code is the visual loader/list surface, which exists because
some Display/UI paths still expose legacy `EffectControl`/`ImageOption` lists.
Native Scene visual loaders now own object, image, and palette startup data.

Recommended Frame/Visual order:

1. Introduce owned `FrameStore`/`FrameGeometry` while keeping behavior compatible
   with `CthughaBuffer`.
2. Split `VideoDirector` so frame composition/filterchain ownership moves toward
   `FrameComposer` and Scene receives only cue/transition-facing ports.
3. Move frame/window sizing for Scene and Audio from `CthughaBuffer::buffer` to
   explicit geometry values.
4. Introduce native visual catalogs/selections for flames, waves, translations,
   palettes, images, border, and flashlight.
5. Point non-Scene visual lists, menus, and runtime config display code at
   native visual catalog data instead of legacy visual option lists.
6. Delete remaining compatibility list surfaces once no production path needs
   legacy visual `EffectControl&` adapters.

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
