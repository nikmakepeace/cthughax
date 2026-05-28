set(CTHUGHA_COMMON_SOURCES
    Option.cc
    OptionInt.cc
    CoreOption.cc
    CoreOptionEntry.cc
    Audio.cc
    AudioFrame.cc
    AudioRuntime.cc
    RuntimeFactory.cc
    PcmSourceFactory.cc
    AudioVisualBridge.cc
    Mixer.cc
    CDPlayer.cc
    imath.cc
    AudioSystem.cc
    IniFiles.cc
    Interface.cc
    info_title_usage.cc
    disp-ncurses.cc
    misc.cc
    DisplayDevice.cc
    AutoChanger.cc
    AudioAnalyzer.cc
    CthughaDisplay.cc
    CthughaFrameBuffer.cc
    Border.cc
    VisualPipeline.cc
    VisualDirector.cc
    keymap.cc
    InterfaceCredits.cc
    InterfaceList.cc
    joystick.cc
    palettes.cc
    pcx.cc
    flames.cc
    waves.cc
    sound_tables.cc
    translate.cc
    AudioProcessor.cc
    CthughaBuffer.cc
    initExitDisp.cc
    Flashlight.cc
    InterfaceHelp.cc
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
        xwin_options.cc
        display.cc
        DisplayDeviceX11.cc
        CthughaDisplayX11.cc
        DisplayDeviceX11-Panel.cc
        nonGL_stubs.cc
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

    if(USE_XPM)
        target_link_libraries(xcthugha PRIVATE ${X11_Xpm_LIB})
    endif()

    if(CURSES_FOUND)
        target_include_directories(xcthugha PRIVATE ${CURSES_INCLUDE_DIRS})
        target_link_libraries(xcthugha PRIVATE ${CURSES_LIBRARIES})
    endif()

    if(WITH_PULSE)
        target_link_libraries(xcthugha PRIVATE ${PULSE_LIBRARIES})
        target_link_directories(xcthugha PRIVATE ${PULSE_LIBRARY_DIRS})
    endif()

    install(TARGETS xcthugha RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_executable(tabheader tabheader.cc)
target_include_directories(tabheader PRIVATE ${CMAKE_BINARY_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS tabheader RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(tabinfo tabinfo.cc)
target_include_directories(tabinfo PRIVATE ${CMAKE_BINARY_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS tabinfo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
