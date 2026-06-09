add_executable(indexed_display_frame_test
    unit/IndexedDisplayFrameTest.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
)

target_include_directories(indexed_display_frame_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(indexed_display_frame_test PRIVATE m)

add_test(NAME indexed_display_frame_test COMMAND indexed_display_frame_test)

add_executable(indexed_frame_test_fixtures_test
    unit/IndexedFrameTestFixturesTest.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
)

target_include_directories(indexed_frame_test_fixtures_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(indexed_frame_test_fixtures_test PRIVATE m)

add_test(NAME indexed_frame_test_fixtures_test COMMAND indexed_frame_test_fixtures_test)


add_executable(palette_transition_test
    unit/PaletteTransitionTest.cc
    ${CMAKE_SOURCE_DIR}/src/PaletteTransition.cc
    ${CMAKE_SOURCE_DIR}/src/FramePalette.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(palette_transition_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(palette_transition_test PRIVATE m)

add_test(NAME palette_transition_test COMMAND palette_transition_test)

add_executable(palette_random_generator_test
    unit/PaletteRandomGeneratorTest.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/PaletteRandomGenerator.cc
)

target_include_directories(palette_random_generator_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(palette_random_generator_test PRIVATE m)

add_test(NAME palette_random_generator_test COMMAND palette_random_generator_test)

add_executable(random_palette_persistence_test
    unit/RandomPalettePersistenceTest.cc
    ${CMAKE_SOURCE_DIR}/src/palettes.cc
    ${CMAKE_SOURCE_DIR}/src/PaletteRandomGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoiceLoader.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
)

target_compile_definitions(random_palette_persistence_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(random_palette_persistence_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(random_palette_persistence_test PRIVATE m)

add_test(NAME random_palette_persistence_test COMMAND random_palette_persistence_test)

add_executable(scene_palette_randomizer_test
    unit/ScenePaletteRandomizerTest.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteRandomizer.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTypedVisualCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTranslationCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/PaletteRandomGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoiceLoader.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(scene_palette_randomizer_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(scene_palette_randomizer_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_palette_randomizer_test PRIVATE m)

add_test(NAME scene_palette_randomizer_test COMMAND scene_palette_randomizer_test)

add_executable(image_placement_test
    unit/ImagePlacementTest.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(image_placement_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(image_placement_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(image_placement_test PRIVATE m)

add_test(NAME image_placement_test COMMAND image_placement_test)

add_executable(general_flame_option_test
    unit/GeneralFlameOptionTest.cc
    ${CMAKE_SOURCE_DIR}/src/flames.cc
    ${CMAKE_SOURCE_DIR}/src/Flame.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(general_flame_option_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(general_flame_option_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(general_flame_option_test PRIVATE m)

add_test(NAME general_flame_option_test COMMAND general_flame_option_test)

add_executable(flame_storage_layout_test
    unit/FlameStorageLayoutTest.cc
    ${CMAKE_SOURCE_DIR}/src/flames.cc
    ${CMAKE_SOURCE_DIR}/src/Flame.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/FrameStore.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeneratorContext.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalysisSnapshot.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(flame_storage_layout_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(flame_storage_layout_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(flame_storage_layout_test PRIVATE m)

add_test(NAME flame_storage_layout_test COMMAND flame_storage_layout_test)

add_executable(translate_storage_layout_test
    unit/TranslateStorageLayoutTest.cc
    ${CMAKE_SOURCE_DIR}/src/Translate.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/FrameStore.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeneratorContext.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalysisSnapshot.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
)

target_include_directories(translate_storage_layout_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(translate_storage_layout_test PRIVATE m)

add_test(NAME translate_storage_layout_test COMMAND translate_storage_layout_test)

add_executable(effect_control_random_test
    unit/EffectControlRandomTest.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeEffectControls.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(effect_control_random_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(effect_control_random_test PRIVATE m)

add_test(NAME effect_control_random_test COMMAND effect_control_random_test)

add_executable(screen_renderer_characterization_test
    unit/ScreenRendererCharacterizationTest.cc
    ${CMAKE_SOURCE_DIR}/src/display.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(screen_renderer_characterization_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(screen_renderer_characterization_test PRIVATE m)

add_test(NAME screen_renderer_characterization_test COMMAND screen_renderer_characterization_test)

add_executable(screen_render_context_test
    unit/ScreenRenderContextTest.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
)

target_include_directories(screen_render_context_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(screen_render_context_test PRIVATE m)

add_test(NAME screen_render_context_test COMMAND screen_render_context_test)

add_executable(frame_completion_test
    unit/FrameCompletionTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
)

target_include_directories(frame_completion_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(frame_completion_test PRIVATE m)

add_test(NAME frame_completion_test COMMAND frame_completion_test)

add_executable(frame_store_test
    unit/FrameStoreTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/FrameStore.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
)

target_include_directories(frame_store_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_store_test PRIVATE m)

add_test(NAME frame_store_test COMMAND frame_store_test)

add_executable(frame_transition_controller_test
    unit/FrameTransitionControllerTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameTransitionController.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
)

target_include_directories(frame_transition_controller_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_transition_controller_test PRIVATE m)

add_test(NAME frame_transition_controller_test
    COMMAND frame_transition_controller_test)

add_executable(frame_generator_boundary_test
    unit/FrameGeneratorBoundaryTest.cc
)

target_compile_definitions(frame_generator_boundary_test PRIVATE
    CTHUGHA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(frame_generator_boundary_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_generator_boundary_test PRIVATE m)

add_test(NAME frame_generator_boundary_test
    COMMAND frame_generator_boundary_test)

add_executable(frame_generator_context_test
    unit/FrameGeneratorContextTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeneratorContext.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalysisSnapshot.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
)

target_include_directories(frame_generator_context_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_generator_context_test PRIVATE m)

add_test(NAME frame_generator_context_test
    COMMAND frame_generator_context_test)

add_executable(frame_filterchain_diagnostics_test
    unit/FrameFilterchainDiagnosticsTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameFilterchain.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(frame_filterchain_diagnostics_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_filterchain_diagnostics_test PRIVATE m)

add_test(NAME frame_filterchain_diagnostics_test
    COMMAND frame_filterchain_diagnostics_test)

add_executable(frame_clock_test
    unit/FrameClockTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameClock.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRateMeter.cc
)

target_include_directories(frame_clock_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_clock_test PRIVATE m)

add_test(NAME frame_clock_test COMMAND frame_clock_test)

add_executable(frame_rate_meter_test
    unit/FrameRateMeterTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRateMeter.cc
)

target_include_directories(frame_rate_meter_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_rate_meter_test PRIVATE m)

add_test(NAME frame_rate_meter_test COMMAND frame_rate_meter_test)

add_executable(frame_pacer_test
    unit/FramePacerTest.cc
    ${CMAKE_SOURCE_DIR}/src/FramePacer.cc
)

target_include_directories(frame_pacer_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_pacer_test PRIVATE m)

add_test(NAME frame_pacer_test COMMAND frame_pacer_test)

add_executable(process_services_test
    unit/ProcessServicesTest.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(process_services_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(process_services_test PRIVATE m)

add_test(NAME process_services_test COMMAND process_services_test)

add_executable(messages_provider_test
    unit/MessagesProviderTest.cc
    ${CMAKE_SOURCE_DIR}/src/DefaultMessagesProvider.cc
    ${CMAKE_SOURCE_DIR}/src/MessageFormatValidator.cc
    ${CMAKE_SOURCE_DIR}/src/MessagesProvider.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(messages_provider_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(messages_provider_test PRIVATE m)

add_test(NAME messages_provider_test COMMAND messages_provider_test)

add_executable(translate_generator_test
    unit/TranslateGeneratorTest.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
)

target_include_directories(translate_generator_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(translate_generator_test PRIVATE m)

add_test(NAME translate_generator_test COMMAND translate_generator_test)

add_executable(frame_generator_frame_budget_test
    unit/FrameGeneratorFrameBudgetTest.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeneratorFrameBudget.cc
)

target_include_directories(frame_generator_frame_budget_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_generator_frame_budget_test PRIVATE m)

add_test(NAME frame_generator_frame_budget_test
    COMMAND frame_generator_frame_budget_test)

add_executable(presentation_composer_test
    unit/PresentationComposerTest.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/FramePalette.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(presentation_composer_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(presentation_composer_test PRIVATE m)

add_test(NAME presentation_composer_test COMMAND presentation_composer_test)

add_executable(cthugha_display_presentation_test
    unit/CthughaDisplayPresentationTest.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayPresentationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/FrameClock.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRateMeter.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/FramePalette.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(cthugha_display_presentation_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(cthugha_display_presentation_test PRIVATE m)

add_test(NAME cthugha_display_presentation_test
    COMMAND cthugha_display_presentation_test)

add_executable(display_geometry_test
    unit/DisplayGeometryTest.cc
)

target_include_directories(display_geometry_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(display_geometry_test PRIVATE m)

add_test(NAME display_geometry_test COMMAND display_geometry_test)

add_executable(display_draw_mode_test
    unit/DisplayDrawModeTest.cc
)

target_include_directories(display_draw_mode_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(display_draw_mode_test PRIVATE m)

add_test(NAME display_draw_mode_test COMMAND display_draw_mode_test)

add_executable(display_refactor_boundary_test
    unit/DisplayRefactorBoundaryTest.cc
)

target_compile_definitions(display_refactor_boundary_test PRIVATE
    CTHUGHA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(display_refactor_boundary_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(display_refactor_boundary_test PRIVATE m)

add_test(NAME display_refactor_boundary_test
    COMMAND display_refactor_boundary_test)

add_executable(viewport_policy_test
    unit/ViewportPolicyTest.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
)

target_include_directories(viewport_policy_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(viewport_policy_test PRIVATE m)

add_test(NAME viewport_policy_test COMMAND viewport_policy_test)

add_executable(viewport_presentation_test
    unit/ViewportPresentationTest.cc
)

target_include_directories(viewport_presentation_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(viewport_presentation_test PRIVATE m)

add_test(NAME viewport_presentation_test COMMAND viewport_presentation_test)

add_executable(cthugha_display_viewport_test
    unit/CthughaDisplayViewportTest.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayPresentationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/FrameClock.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRateMeter.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(cthugha_display_viewport_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/unit
)

target_link_libraries(cthugha_display_viewport_test PRIVATE m)

add_test(NAME cthugha_display_viewport_test
    COMMAND cthugha_display_viewport_test)

add_executable(display_runtime_test
    unit/DisplayRuntimeTest.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/OverlaySource.cc
    ${CMAKE_SOURCE_DIR}/src/PixelTransfer.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/FramePalette.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
)

target_include_directories(display_runtime_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(display_runtime_test PRIVATE m)

add_test(NAME display_runtime_test COMMAND display_runtime_test)

add_executable(input_queue_test
    unit/InputQueueTest.cc
    ${CMAKE_SOURCE_DIR}/src/InputQueue.cc
    ${CMAKE_SOURCE_DIR}/src/keys.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
)

target_include_directories(input_queue_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(input_queue_test PRIVATE m)

add_test(NAME input_queue_test COMMAND input_queue_test)

add_executable(commands_input_runtime_test
    unit/CommandsInputRuntimeTest.cc
    ${CMAKE_SOURCE_DIR}/src/CommandsInputRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayPresentationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/InterfaceRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/InputQueue.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/keys.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(commands_input_runtime_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(commands_input_runtime_test PRIVATE m)

add_test(NAME commands_input_runtime_test COMMAND commands_input_runtime_test)

add_executable(display_ownership_test
    unit/DisplayOwnershipTest.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayPresentationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/DisplaySystem.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
    ${CMAKE_SOURCE_DIR}/src/FrameClock.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRateMeter.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/FrameRenderTarget.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(display_ownership_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(display_ownership_test PRIVATE m)

add_test(NAME display_ownership_test COMMAND display_ownership_test)

add_executable(overlay_source_test
    unit/OverlaySourceTest.cc
    ${CMAKE_SOURCE_DIR}/src/OverlaySource.cc
)

target_include_directories(overlay_source_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(overlay_source_test PRIVATE m)

add_test(NAME overlay_source_test COMMAND overlay_source_test)

add_executable(fps_overlay_test
    unit/FpsOverlayTest.cc
    ${CMAKE_SOURCE_DIR}/src/FpsOverlay.cc
    ${CMAKE_SOURCE_DIR}/src/OverlaySource.cc
)

target_include_directories(fps_overlay_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fps_overlay_test PRIVATE m)

add_test(NAME fps_overlay_test COMMAND fps_overlay_test)

add_executable(pixel_transfer_test
    unit/PixelTransferTest.cc
    ${CMAKE_SOURCE_DIR}/src/PixelTransfer.cc
)

target_include_directories(pixel_transfer_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pixel_transfer_test PRIVATE m)

add_test(NAME pixel_transfer_test COMMAND pixel_transfer_test)

add_executable(warning_clean_smoke_test
    unit/WarningCleanSmokeTest.cc
)

target_include_directories(warning_clean_smoke_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(warning_clean_smoke_test PRIVATE
        -Werror=macro-redefined
        -Werror=deprecated-declarations
    )
endif()

add_test(NAME warning_clean_smoke_test COMMAND warning_clean_smoke_test)

add_executable(configuration_test
    unit/ConfigurationTest.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
)

target_include_directories(configuration_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(configuration_test PRIVATE m)

add_test(NAME configuration_test COMMAND configuration_test)

add_executable(startup_configuration_test
    unit/StartupConfigurationTest.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/StartupConfiguration.cc
)

target_include_directories(startup_configuration_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(startup_configuration_test PRIVATE m)

add_test(NAME startup_configuration_test COMMAND startup_configuration_test)

if(CTH_BUILD_X11)
    add_executable(x11_configuration_test
        unit/X11ConfigurationTest.cc
        ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    )

    target_compile_definitions(x11_configuration_test PRIVATE CTH_XWIN)

    target_include_directories(x11_configuration_test PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(x11_configuration_test PRIVATE m)

    add_test(NAME x11_configuration_test COMMAND x11_configuration_test)
endif()

add_executable(configuration_dependency_boundary_test
    unit/ConfigurationDependencyBoundaryTest.cc
)

target_compile_definitions(configuration_dependency_boundary_test PRIVATE
    CTHUGHA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(configuration_dependency_boundary_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(configuration_dependency_boundary_test PRIVATE m)

add_test(NAME configuration_dependency_boundary_test
    COMMAND configuration_dependency_boundary_test)

add_executable(wave_object_test
    unit/WaveObjectTest.cc
    ${CMAKE_SOURCE_DIR}/src/WaveObject.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(wave_object_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(wave_object_test PRIVATE m)

add_test(NAME wave_object_test COMMAND wave_object_test)

add_executable(wave_runtime_random_test
    unit/WaveRuntimeRandomTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalyzer.cc
    ${CMAKE_SOURCE_DIR}/src/Wave.cc
    ${CMAKE_SOURCE_DIR}/src/FrameGeneratorContext.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalysisSnapshot.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(wave_runtime_random_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(wave_runtime_random_test PRIVATE m)

add_test(NAME wave_runtime_random_test COMMAND wave_runtime_random_test)

add_executable(effect_preset_catalog_test
    unit/EffectPresetCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(effect_preset_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(effect_preset_catalog_test PRIVATE m)

add_test(NAME effect_preset_catalog_test COMMAND effect_preset_catalog_test)

add_executable(runtime_command_test
    unit/RuntimeCommandTest.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
)

target_include_directories(runtime_command_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_command_test PRIVATE m)

add_test(NAME runtime_command_test COMMAND runtime_command_test)

add_executable(scene_choice_list_catalog_test
    unit/SceneChoiceListCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceListCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
)

target_include_directories(scene_choice_list_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_choice_list_catalog_test PRIVATE m)

add_test(NAME scene_choice_list_catalog_test
    COMMAND scene_choice_list_catalog_test)

add_executable(scene_built_in_choice_catalogs_test
    unit/SceneBuiltInChoiceCatalogsTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneBuiltInChoiceCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTypedVisualCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTranslationCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceListCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(scene_built_in_choice_catalogs_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(scene_built_in_choice_catalogs_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_built_in_choice_catalogs_test PRIVATE m)

add_test(NAME scene_built_in_choice_catalogs_test
    COMMAND scene_built_in_choice_catalogs_test)

add_executable(scene_translation_catalog_test
    unit/SceneTranslationCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTranslationCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
)

target_include_directories(scene_translation_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_translation_catalog_test PRIVATE m)

add_test(NAME scene_translation_catalog_test
    COMMAND scene_translation_catalog_test)

add_executable(scene_wave_object_catalog_test
    unit/SceneWaveObjectCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
)

target_include_directories(scene_wave_object_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_wave_object_catalog_test PRIVATE m)

add_test(NAME scene_wave_object_catalog_test
    COMMAND scene_wave_object_catalog_test)

add_executable(scene_image_catalog_test
    unit/SceneImageCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(scene_image_catalog_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(scene_image_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_image_catalog_test PRIVATE m)

add_test(NAME scene_image_catalog_test
    COMMAND scene_image_catalog_test)

add_executable(scene_palette_catalog_test
    unit/ScenePaletteCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
)

target_include_directories(scene_palette_catalog_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_palette_catalog_test PRIVATE m)

add_test(NAME scene_palette_catalog_test
    COMMAND scene_palette_catalog_test)

add_executable(scene_catalog_loader_test
    unit/SceneCatalogLoaderTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalogLoader.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/WaveObject.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalogLoader.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalogLoader.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(scene_catalog_loader_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(scene_catalog_loader_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_catalog_loader_test PRIVATE m)

add_test(NAME scene_catalog_loader_test
    COMMAND scene_catalog_loader_test)

add_executable(scene_choice_lock_value_test
    unit/SceneChoiceLockValueTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
)

target_include_directories(scene_choice_lock_value_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_choice_lock_value_test PRIVATE m)

add_test(NAME scene_choice_lock_value_test
    COMMAND scene_choice_lock_value_test)

add_executable(scene_visual_selection_factory_test
    unit/SceneVisualSelectionFactoryTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelectionFactory.cc
    ${CMAKE_SOURCE_DIR}/src/SceneBuiltInChoiceCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceListCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTypedVisualCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelectionSet.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeneralFlameSelectionValue.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTranslationCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(scene_visual_selection_factory_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(scene_visual_selection_factory_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_link_libraries(scene_visual_selection_factory_test PRIVATE m)

add_test(NAME scene_visual_selection_factory_test
    COMMAND scene_visual_selection_factory_test)

add_executable(scene_typed_visual_catalogs_test
    unit/SceneTypedVisualCatalogsTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTypedVisualCatalogs.cc
    ${CMAKE_SOURCE_DIR}/src/SceneWaveObjectCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneTranslationCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/ScenePaletteCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneImageCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/ColorPalette.cc
    ${CMAKE_SOURCE_DIR}/src/Image.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(scene_typed_visual_catalogs_test PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(scene_typed_visual_catalogs_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_typed_visual_catalogs_test PRIVATE m)

add_test(NAME scene_typed_visual_catalogs_test
    COMMAND scene_typed_visual_catalogs_test)

add_executable(scene_visual_selection_set_test
    unit/SceneVisualSelectionSetTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelectionSet.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
)

target_include_directories(scene_visual_selection_set_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_visual_selection_set_test PRIVATE m)

add_test(NAME scene_visual_selection_set_test
    COMMAND scene_visual_selection_set_test)

add_executable(scene_general_flame_selection_value_test
    unit/SceneGeneralFlameSelectionValueTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeneralFlameSelectionValue.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(scene_general_flame_selection_value_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_general_flame_selection_value_test PRIVATE m)

add_test(NAME scene_general_flame_selection_value_test
    COMMAND scene_general_flame_selection_value_test)

add_executable(scene_commands_sync_test
    unit/SceneCommandsSyncTest.cc
    ${CMAKE_SOURCE_DIR}/src/Scene.cc
    ${CMAKE_SOURCE_DIR}/src/SceneGeometry.cc
    ${CMAKE_SOURCE_DIR}/src/SceneDependencies.cc
    ${CMAKE_SOURCE_DIR}/src/SceneRuntimeDependencies.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChoiceSelection.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
)

target_include_directories(scene_commands_sync_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_commands_sync_test PRIVATE m)

add_test(NAME scene_commands_sync_test COMMAND scene_commands_sync_test)

add_executable(mixer_session_test
    unit/MixerSessionTest.cc
    ${CMAKE_SOURCE_DIR}/src/Mixer.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
)

target_include_directories(mixer_session_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mixer_session_test PRIVATE m)

add_test(NAME mixer_session_test COMMAND mixer_session_test)

add_executable(mixer_controls_test
    unit/MixerControlsTest.cc
    ${CMAKE_SOURCE_DIR}/src/Mixer.cc
    ${CMAKE_SOURCE_DIR}/src/MixerControls.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
)

target_include_directories(mixer_controls_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mixer_controls_test PRIVATE m)

add_test(NAME mixer_controls_test COMMAND mixer_controls_test)

add_executable(audio_decoded_history_test
    unit/AudioDecodedHistoryTest.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(audio_decoded_history_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_decoded_history_test PRIVATE m)

add_test(NAME audio_decoded_history_test COMMAND audio_decoded_history_test)

add_executable(audio_passthrough_test
    unit/AudioPassthroughTest.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioPassthrough.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(audio_passthrough_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_passthrough_test PRIVATE Threads::Threads m)

add_test(NAME audio_passthrough_test COMMAND audio_passthrough_test)

add_executable(audio_ingest_test
    unit/AudioIngestTest.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioIngest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioPassthrough.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(audio_ingest_test PRIVATE
    CTH_AUDIO_INGEST_NO_CONFIG
)

target_include_directories(audio_ingest_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_ingest_test PRIVATE Threads::Threads m)

add_test(NAME audio_ingest_test COMMAND audio_ingest_test)

add_executable(audio_device_negotiation_test
    unit/AudioDeviceNegotiationTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioDSPOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioSettings.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/DspPcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSourceFactory.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

if(WITH_MINIAUDIO)
    target_sources(audio_device_negotiation_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src/MiniAudioCapturePcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
    )
endif()

target_compile_definitions(audio_device_negotiation_test PRIVATE
    CTH_AUDIO_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/audio"
)

target_include_directories(audio_device_negotiation_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_device_negotiation_test PRIVATE m)

if(WITH_MINIAUDIO)
    target_link_libraries(audio_device_negotiation_test PRIVATE
        cthugha_miniaudio
    )
endif()

add_test(NAME audio_device_negotiation_test COMMAND audio_device_negotiation_test)

add_executable(runtime_factory_audio_output_test
    unit/RuntimeFactoryAudioOutputTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioDSPOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioPulseOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/DspPcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSourceFactory.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeFactory.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

if(WITH_MINIAUDIO)
    target_sources(runtime_factory_audio_output_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src/AudioMiniAudioOutput.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioCapturePcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
    )
endif()

target_include_directories(runtime_factory_audio_output_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${PULSE_INCLUDE_DIRS}
)

target_compile_options(runtime_factory_audio_output_test PRIVATE
    ${PULSE_CFLAGS_OTHER}
)

target_compile_definitions(runtime_factory_audio_output_test PRIVATE
    RUNTIME_FACTORY_TEST_HOOKS
)

target_link_libraries(runtime_factory_audio_output_test PRIVATE
    Threads::Threads
    m
)

if(WITH_PULSE)
    target_link_libraries(runtime_factory_audio_output_test PRIVATE
        ${PULSE_LIBRARIES}
    )
    target_link_directories(runtime_factory_audio_output_test PRIVATE
        ${PULSE_LIBRARY_DIRS}
    )
endif()

if(WITH_MINIAUDIO)
    target_link_libraries(runtime_factory_audio_output_test PRIVATE
        cthugha_miniaudio
    )
endif()

add_test(NAME runtime_factory_audio_output_test
    COMMAND runtime_factory_audio_output_test)

if(WITH_MINIAUDIO)
    add_executable(miniaudio_capture_pcm_source_test
        unit/MiniAudioCapturePcmSourceTest.cc
        ${CMAKE_SOURCE_DIR}/src/AudioSettings.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioCapturePcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
        ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
        ${CMAKE_SOURCE_DIR}/src/imath.cc
    )

    target_compile_definitions(miniaudio_capture_pcm_source_test PRIVATE
        AUDIO_CAPTURE_TEST_HOOKS
    )

    target_include_directories(miniaudio_capture_pcm_source_test PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(miniaudio_capture_pcm_source_test PRIVATE
        cthugha_miniaudio
        m
    )

    add_test(NAME miniaudio_capture_pcm_source_test
        COMMAND miniaudio_capture_pcm_source_test)
endif()

add_executable(audio_output_config_test
    unit/AudioOutputConfigTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioDSPOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioPulseOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

if(WITH_MINIAUDIO)
    target_sources(audio_output_config_test PRIVATE
        ${CMAKE_SOURCE_DIR}/src/AudioMiniAudioOutput.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
        ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    )
    target_compile_definitions(audio_output_config_test PRIVATE
        AUDIO_OUTPUT_TEST_HOOKS
        CTH_AUDIO_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/audio"
    )
endif()

target_include_directories(audio_output_config_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${PULSE_INCLUDE_DIRS}
)

target_compile_options(audio_output_config_test PRIVATE ${PULSE_CFLAGS_OTHER})

target_link_libraries(audio_output_config_test PRIVATE m)

if(WITH_PULSE)
    target_link_libraries(audio_output_config_test PRIVATE ${PULSE_LIBRARIES})
    target_link_directories(audio_output_config_test PRIVATE ${PULSE_LIBRARY_DIRS})
endif()

if(WITH_MINIAUDIO)
    target_link_libraries(audio_output_config_test PRIVATE cthugha_miniaudio)
endif()

add_test(NAME audio_output_config_test COMMAND audio_output_config_test)

add_executable(audio_output_dump_test
    unit/AudioOutputDumpTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(audio_output_dump_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_output_dump_test PRIVATE m)

add_test(NAME audio_output_dump_test COMMAND audio_output_dump_test)

if(WITH_MINIAUDIO)
    set(MINIAUDIO_SMOKE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
        ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
        ${CMAKE_SOURCE_DIR}/src/AudioMiniAudioOutput.cc
        ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
        ${CMAKE_SOURCE_DIR}/src/AudioOutputConfig.cc
        ${CMAKE_SOURCE_DIR}/src/AudioOutputDump.cc
        ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
        ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
        ${CMAKE_SOURCE_DIR}/src/imath.cc
    )

    add_executable(miniaudio_playback_open_smoke
        smoke/MiniAudioPlaybackOpenSmoke.cc
        ${MINIAUDIO_SMOKE_SOURCES}
    )

    add_executable(miniaudio_sine_smoke
        smoke/MiniAudioSineSmoke.cc
        ${MINIAUDIO_SMOKE_SOURCES}
    )

    add_executable(miniaudio_file_passthrough_smoke
        smoke/MiniAudioFilePassthroughSmoke.cc
        ${MINIAUDIO_SMOKE_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    )

    add_executable(miniaudio_capture_smoke
        smoke/MiniAudioCaptureSmoke.cc
        ${CMAKE_SOURCE_DIR}/src/AudioSettings.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioCapturePcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/MiniAudioImplementation.cc
        ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
        ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
        ${CMAKE_SOURCE_DIR}/src/imath.cc
    )

    foreach(MINIAUDIO_SMOKE_TARGET
        miniaudio_playback_open_smoke
        miniaudio_sine_smoke
        miniaudio_file_passthrough_smoke
        miniaudio_capture_smoke)
        target_include_directories(${MINIAUDIO_SMOKE_TARGET} PRIVATE
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/src
            ${CMAKE_SOURCE_DIR}/src
        )

        target_link_libraries(${MINIAUDIO_SMOKE_TARGET} PRIVATE
            cthugha_miniaudio
            Threads::Threads
            m
        )
    endforeach()

    target_compile_definitions(miniaudio_file_passthrough_smoke PRIVATE
        CTH_AUDIO_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/audio"
    )

    if(CTH_RUN_AUDIO_DEVICE_TESTS)
        add_test(NAME miniaudio_playback_open_smoke
            COMMAND miniaudio_playback_open_smoke)
        add_test(NAME miniaudio_sine_smoke
            COMMAND miniaudio_sine_smoke)
        add_test(NAME miniaudio_file_passthrough_smoke
            COMMAND miniaudio_file_passthrough_smoke)
        add_test(NAME miniaudio_capture_smoke
            COMMAND miniaudio_capture_smoke)
    endif()
endif()

add_executable(audio_frame_pipeline_test
    unit/AudioFramePipelineTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalyzer.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFramePipeline.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFftProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessingState.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessingSelector.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(audio_frame_pipeline_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_frame_pipeline_test PRIVATE m)

add_test(NAME audio_frame_pipeline_test COMMAND audio_frame_pipeline_test)

add_executable(interface_runtime_test
    unit/InterfaceRuntimeTest.cc
    ${CMAKE_SOURCE_DIR}/src/InterfaceRuntime.cc
    ${CMAKE_SOURCE_DIR}/src/SceneVisualSelections.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
)

target_include_directories(interface_runtime_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME interface_runtime_test COMMAND interface_runtime_test)

add_executable(audio_frame_processor_test
    unit/AudioFrameProcessorTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFftProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_compile_definitions(audio_frame_processor_test PRIVATE
    CTH_AUDIO_FRAME_NO_RUNTIME
)

target_include_directories(audio_frame_processor_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_frame_processor_test PRIVATE m)

add_test(NAME audio_frame_processor_test COMMAND audio_frame_processor_test)

add_executable(runtime_auto_change_controls_test
    unit/RuntimeAutoChangeControlsTest.cc
    ${CMAKE_SOURCE_DIR}/src/AutoChangeControls.cc
    ${CMAKE_SOURCE_DIR}/src/AutoChangeSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeAutoChangeControls.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(runtime_auto_change_controls_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_auto_change_controls_test PRIVATE m)

add_test(NAME runtime_auto_change_controls_test
    COMMAND runtime_auto_change_controls_test)

add_executable(runtime_audio_controls_test
    unit/RuntimeAudioControlsTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFftProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessingState.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/Mixer.cc
    ${CMAKE_SOURCE_DIR}/src/MixerControls.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeAudioControls.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
)

target_include_directories(runtime_audio_controls_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_audio_controls_test PRIVATE m)

add_test(NAME runtime_audio_controls_test COMMAND runtime_audio_controls_test)

add_executable(runtime_config_selection_test
    unit/RuntimeConfigSelectionTest.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeConfigSelection.cc
)

target_include_directories(runtime_config_selection_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_config_selection_test PRIVATE m)

add_test(NAME runtime_config_selection_test
    COMMAND runtime_config_selection_test)

add_executable(runtime_config_contributors_test
    unit/RuntimeConfigContributorsTest.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessingState.cc
    ${CMAKE_SOURCE_DIR}/src/AutoChangeSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/DisplayPresentationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeConfigContributors.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeConfigRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(runtime_config_contributors_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_config_contributors_test PRIVATE m)

add_test(NAME runtime_config_contributors_test
    COMMAND runtime_config_contributors_test)

add_executable(runtime_persistence_test
    unit/RuntimePersistenceTest.cc
    ${CMAKE_SOURCE_DIR}/src/AutoChangeSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeConfigRegistry.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimePersistence.cc
)

target_include_directories(runtime_persistence_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_persistence_test PRIVATE m)

add_test(NAME runtime_persistence_test COMMAND runtime_persistence_test)

add_executable(runtime_shutdown_test
    unit/RuntimeShutdownTest.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeShutdown.cc
)

target_include_directories(runtime_shutdown_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_shutdown_test PRIVATE m)

add_test(NAME runtime_shutdown_test COMMAND runtime_shutdown_test)

add_executable(runtime_change_mediator_test
    unit/RuntimeChangeMediatorTest.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommand.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeCommandTargets.cc
    ${CMAKE_SOURCE_DIR}/src/RuntimeChangeMediator.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(runtime_change_mediator_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(runtime_change_mediator_test PRIVATE m)

add_test(NAME runtime_change_mediator_test COMMAND runtime_change_mediator_test)

add_executable(scene_change_scheduler_test
    unit/SceneChangeSchedulerTest.cc
    ${CMAKE_SOURCE_DIR}/src/SceneChangeScheduler.cc
    ${CMAKE_SOURCE_DIR}/src/AutoChangeSettings.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/ProcessServices.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
)

target_include_directories(scene_change_scheduler_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(scene_change_scheduler_test PRIVATE m)

add_test(NAME scene_change_scheduler_test COMMAND scene_change_scheduler_test)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_library(format_warning_clean OBJECT
        ${CMAKE_SOURCE_DIR}/src/SceneChangeScheduler.cc
        ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
        ${CMAKE_SOURCE_DIR}/src/EffectChoiceLoader.cc
        ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
        ${CMAKE_SOURCE_DIR}/src/EffectRegistry.cc
        ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
        ${CMAKE_SOURCE_DIR}/src/Interface.cc
        ${CMAKE_SOURCE_DIR}/src/InterfaceList.cc
        ${CMAKE_SOURCE_DIR}/src/flames.cc
        ${CMAKE_SOURCE_DIR}/src/info_title_usage.cc
        ${CMAKE_SOURCE_DIR}/src/LegacyLoggingAdapter.cc
        ${CMAKE_SOURCE_DIR}/src/palettes.cc
    )

    target_compile_definitions(format_warning_clean PRIVATE
        CTH_LIBDIR="${CTH_DATA_DIR}"
    )

    target_include_directories(format_warning_clean PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_SOURCE_DIR}/src
        ${X11_INCLUDE_DIR}
        ${PULSE_INCLUDE_DIRS}
    )

    target_compile_options(format_warning_clean PRIVATE
        -Werror=deprecated-declarations
    )

    add_test(NAME format_warning_clean_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format_warning_clean
    )

    if(CTH_BUILD_X11)
        add_library(xaw_warning_clean OBJECT
            ${CMAKE_SOURCE_DIR}/src/DisplayDeviceX11-Panel.cc
        )

        target_compile_definitions(xaw_warning_clean PRIVATE
            CTH_LIBDIR="${CTH_DATA_DIR}"
        )

        target_include_directories(xaw_warning_clean PRIVATE
            ${CMAKE_BINARY_DIR}
            ${CMAKE_BINARY_DIR}/src
            ${CMAKE_SOURCE_DIR}/src
            ${X11_INCLUDE_DIR}
        )

        target_compile_options(xaw_warning_clean PRIVATE
            -Werror=writable-strings
        )

        add_test(NAME xaw_warning_clean_build
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target xaw_warning_clean
        )
    endif()

    add_library(waves_warning_clean OBJECT
        ${CMAKE_SOURCE_DIR}/src/waves.cc
    )

    target_compile_definitions(waves_warning_clean PRIVATE
        CTH_LIBDIR="${CTH_DATA_DIR}"
    )

    target_include_directories(waves_warning_clean PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_options(waves_warning_clean PRIVATE
        -Werror
    )

    add_test(NAME waves_warning_clean_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target waves_warning_clean
    )
endif()
