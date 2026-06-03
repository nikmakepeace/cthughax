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
)

target_include_directories(palette_transition_test PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(palette_transition_test PRIVATE m)

add_test(NAME palette_transition_test COMMAND palette_transition_test)

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

add_executable(presentation_composer_test
    unit/PresentationComposerTest.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
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
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaBuffer.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
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

add_executable(cthugha_display_viewport_test
    unit/CthughaDisplayViewportTest.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
    ${CMAKE_SOURCE_DIR}/src/PresentationComposer.cc
    ${CMAKE_SOURCE_DIR}/src/FrameCompletion.cc
    ${CMAKE_SOURCE_DIR}/src/ViewportPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/Screen.cc
    ${CMAKE_SOURCE_DIR}/src/ScreenRenderContext.cc
    ${CMAKE_SOURCE_DIR}/src/IndexedDisplayFrame.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaBuffer.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
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

add_executable(effect_preset_catalog_test
    unit/EffectPresetCatalogTest.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
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

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_library(format_warning_clean OBJECT
        ${CMAKE_SOURCE_DIR}/src/AutoChanger.cc
        ${CMAKE_SOURCE_DIR}/src/CthughaDisplay.cc
        ${CMAKE_SOURCE_DIR}/src/EffectChoiceLoader.cc
        ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
        ${CMAKE_SOURCE_DIR}/src/Interface.cc
        ${CMAKE_SOURCE_DIR}/src/InterfaceList.cc
        ${CMAKE_SOURCE_DIR}/src/Screenshot.cc
        ${CMAKE_SOURCE_DIR}/src/flames.cc
        ${CMAKE_SOURCE_DIR}/src/info_title_usage.cc
        ${CMAKE_SOURCE_DIR}/src/misc.cc
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
