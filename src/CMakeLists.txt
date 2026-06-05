set(CTHUGHA_COMMON_SOURCES
    Option.cc
    OptionInt.cc
    EffectControl.cc
    EffectControlPolicy.cc
    EffectPresetCatalog.cc
    EffectChoiceLoader.cc
    EffectChoice.cc
    AudioInternal.cc
    AudioOutput.cc
    AudioPulseOutput.cc
    AudioDSPOutput.cc
    DecodedAudioHistory.cc
    AudioPassthrough.cc
    AudioIngest.cc
    PcmSource.cc
    DspPcmSource.cc
    AudioFrame.cc
    AudioSettings.cc
    RuntimeFactory.cc
    Scene.cc
    RuntimeCommand.cc
    RuntimeDisplayControls.cc
    RuntimeAudioControls.cc
    RuntimeAutoChangeControls.cc
    RuntimeEffectControls.cc
    RuntimePersistence.cc
    RuntimeShutdown.cc
    RuntimeChangeMediator.cc
    Screen.cc
    ScreenRenderContext.cc
    DefaultMessagesProvider.cc
    FileMessagesProvider.cc
    MessageFormatValidator.cc
    MessagesProvider.cc
    QotdMessagesProvider.cc
    SilenceMessage.cc
    PcmSourceFactory.cc
    AudioVisualBridge.cc
    PlatformLifecycle.cc
    Mixer.cc
    imath.cc
    AudioSystem.cc
    Configuration.cc
    StartupConfiguration.cc
    RuntimeConfigRegistry.cc
    RuntimeConfigSelection.cc
    LegacyRuntimeConfigContributor.cc
    IniFiles.cc
    Interface.cc
    info_title_usage.cc
    misc.cc
    DisplayDevice.cc
    DisplayRuntime.cc
    DisplayRuntimeOwnership.cc
    AutoChanger.cc
    AudioAnalyzer.cc
    CthughaDisplay.cc
    Border.cc
    ColorPalette.cc
    DosVga9x14Font.cc
    FrameClock.cc
    FrameCompletion.cc
    FramePacer.cc
    FrameRateMeter.cc
    Image.cc
    IndexedDisplayFrame.cc
    OverlaySource.cc
    PixelTransfer.cc
    PresentationComposer.cc
    ViewportPolicy.cc
    VideoFilterchain.cc
    VideoFilterchainSequence.cc
    VideoFilters.cc
    VideoFilterchainFactory.cc
    VideoFrameBudget.cc
    VideoDirector.cc
    keymap.cc
    InterfaceCredits.cc
    InterfaceList.cc
    palettes.cc
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
    AudioProcessor.cc
    CthughaBuffer.cc
    Application.cc
    main.cc
    Flashlight.cc
    InterfaceHelp.cc
    FpsOverlay.cc
)

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

    install(TARGETS xcthugha RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
