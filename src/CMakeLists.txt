set(CTHUGHA_COMMON_SOURCES
    Option.cc
    OptionInt.cc
    EffectControl.cc
    EffectRegistry.cc
    EffectControlPolicy.cc
    EffectPresetCatalog.cc
    EffectChoiceLoader.cc
    EffectChoice.cc
    AudioInternal.cc
    AudioOutputConfig.cc
    AudioOutputDump.cc
    AudioOutput.cc
    AudioPulseOutput.cc
    AudioDSPOutput.cc
    DecodedAudioHistory.cc
    AudioPassthrough.cc
    AudioIngest.cc
    PcmSource.cc
    DspPcmSource.cc
    AudioFrame.cc
    AudioFramePipeline.cc
    AudioSettings.cc
    AutoChangeSettings.cc
    AutoChangeControls.cc
    RuntimeFactory.cc
    Scene.cc
    SceneGeometry.cc
    SceneDependencies.cc
    SceneRuntimeDependencies.cc
    SceneChoiceListCatalog.cc
    SceneBuiltInChoiceCatalogs.cc
    SceneChoiceSelection.cc
    SceneGeneralFlameSelectionValue.cc
    SceneImageCatalog.cc
    ScenePaletteCatalog.cc
    SceneTranslationCatalog.cc
    SceneWaveObjectCatalog.cc
    SceneTypedVisualCatalogs.cc
    SceneVisualSelectionSet.cc
    SceneVisualSelections.cc
    ScenePaletteRandomizer.cc
    SceneImageCatalogLoader.cc
    ScenePaletteCatalogLoader.cc
    SceneWaveObjectCatalogLoader.cc
    SceneVisualSelectionFactory.cc
    SceneVisualCatalogServiceFactory.cc
    SceneVisualCatalogService.cc
    SceneRuntime.cc
    SceneSerializer.cc
    RuntimeCommand.cc
    RuntimeDisplayControls.cc
    RuntimeAudioControls.cc
    RuntimeAutoChangeControls.cc
    RuntimeEffectControls.cc
    RuntimeCommandTargets.cc
    RuntimePersistence.cc
    RuntimeShutdown.cc
    RuntimeChangeMediator.cc
    ProcessServices.cc
    Screen.cc
    ScreenRenderContext.cc
    DefaultMessagesProvider.cc
    FileMessagesProvider.cc
    MessageFormatValidator.cc
    MessagesProvider.cc
    QotdMessagesProvider.cc
    SilenceMessage.cc
    PcmSourceFactory.cc
    PlatformLifecycle.cc
    Mixer.cc
    MixerControls.cc
    MixerInterfaceControls.cc
    imath.cc
    AudioSystem.cc
    Configuration.cc
    StartupConfiguration.cc
    RuntimeConfigRegistry.cc
    RuntimeConfigContributors.cc
    RuntimeConfigSelection.cc
    LegacyRuntimeConfigContributor.cc
    IniFiles.cc
    CommandsInputRuntime.cc
    InputQueue.cc
    Interface.cc
    InterfaceRuntime.cc
    info_title_usage.cc
    LegacyLoggingAdapter.cc
    DisplayDevice.cc
    DisplayPresentationOptions.cc
    DisplayRuntime.cc
    DisplaySystem.cc
    SceneChangeScheduler.cc
    AudioAnalyzer.cc
    AudioAnalysisSnapshot.cc
    CthughaDisplay.cc
    Border.cc
    ColorPalette.cc
    DosVga9x14Font.cc
    FrameClock.cc
    FrameCompletion.cc
    FrameGeneratorPipeline.cc
    FrameGeneratorContext.cc
    FrameRenderContext.cc
    FrameGeneratorRuntime.cc
    FrameGeneratorSceneBinding.cc
    FrameGeometry.cc
    FramePacer.cc
    FrameRateMeter.cc
    FrameStore.cc
    FrameTransitionController.cc
    Image.cc
    IndexedDisplayFrame.cc
    OverlaySource.cc
    PixelTransfer.cc
    PresentationComposer.cc
    ViewportPolicy.cc
    FrameFilterchain.cc
    FrameFilterchainSequence.cc
    FrameFilters.cc
    FrameFilterchainFactory.cc
    FrameGeneratorFrameBudget.cc
    keymap.cc
    InterfaceCredits.cc
    InterfaceList.cc
    palettes.cc
    PaletteRandomGenerator.cc
    pcx.cc
    png.cc
    Flame.cc
    flames.cc
    FramePalette.cc
    PaletteTransition.cc
    Translate.cc
    TranslateGenerator.cc
    Wave.cc
    WaveObject.cc
    waves.cc
    sound_tables.cc
    TranslationOptions.cc
    AudioProcessingState.cc
    AudioProcessingSelector.cc
    AudioProcessor.cc
    AudioFftProcessor.cc
    FrameRenderTarget.cc
    ApplicationDisplayFrontend.cc
    Application.cc
    main.cc
    Flashlight.cc
    InterfaceHelp.cc
    FpsOverlay.cc
)

if(WITH_MINIAUDIO)
    list(APPEND CTHUGHA_COMMON_SOURCES
        AudioMiniAudioOutput.cc
        MiniAudioCapturePcmSource.cc
        MiniAudioImplementation.cc
    )
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.keymap.str
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/default.keymap
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/default.keymap.str
        -P ${CMAKE_SOURCE_DIR}/cmake/generate_keymap.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/default.keymap
    VERBATIM
)

add_custom_target(default_keymap_str DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/default.keymap.str)

if(CTH_BUILD_X11)
    add_executable(xcthugha
        ${CTHUGHA_COMMON_SOURCES}
        xwin_keys.cc
        display.cc
        DisplayDeviceX11.cc
        CthughaDisplayX11.cc
        DisplayDeviceX11-Panel.cc
        ${CMAKE_CURRENT_BINARY_DIR}/default.keymap.str
    )

    add_dependencies(xcthugha default_keymap_str)

    target_compile_definitions(xcthugha PRIVATE
        CTH_XWIN
        CTH_LIBDIR="${CTH_DATA_DIR}"
    )

    target_include_directories(xcthugha PRIVATE
        ${CMAKE_BINARY_DIR}/src
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${X11_INCLUDE_DIR}
        ${PULSE_INCLUDE_DIRS}
    )

    target_compile_options(xcthugha PRIVATE ${PULSE_CFLAGS_OTHER})

    target_link_libraries(xcthugha PRIVATE
        ${X11_LIBRARIES}
        ${X11_Xext_LIB}
        ${X11_Xt_LIB}
        ${X11_Xaw_LIB}
        ${X11_Xmu_LIB}
        Threads::Threads
        ZLIB::ZLIB
        m
    )

    if(WITH_PULSE)
        target_link_libraries(xcthugha PRIVATE ${PULSE_LIBRARIES})
        target_link_directories(xcthugha PRIVATE ${PULSE_LIBRARY_DIRS})
    endif()

    if(WITH_MINIAUDIO)
        target_link_libraries(xcthugha PRIVATE cthugha_miniaudio)
    endif()

    install(TARGETS xcthugha RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
