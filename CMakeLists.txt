cmake_minimum_required(VERSION 3.16)

project("Cthugha X" VERSION 0.9 LANGUAGES C CXX)

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(GNUInstallDirs)
include(TestBigEndian)

option(CTH_BUILD_X11 "Build the modern X11 frontend" ON)
option(CTH_BUILD_SDL3 "Build the SDL3 display driver" OFF)
option(CTH_BUILD_TESTS "Build and register focused unit tests" ON)
option(CTH_BUILD_BENCHMARKS "Build Google Benchmark performance suites" OFF)
option(CTH_RUN_AUDIO_DEVICE_TESTS "Register opt-in tests that open real audio devices" OFF)
option(CTH_ENABLE_PULSE "Enable PulseAudio-compatible output when libpulse-simple is available" ON)
option(CTH_ENABLE_DSP "Enable OSS /dev/dsp support when soundcard headers are available" ON)
option(CTH_ENABLE_MIXER "Enable OSS mixer controls when soundcard headers are available" ON)
option(CTH_ENABLE_MINIMP3 "Enable embedded minimp3 decoding" ON)
option(CTH_ENABLE_MINIAUDIO "Enable miniaudio playback/capture devices" ON)
option(CTH_MINIAUDIO_NO_RUNTIME_LINKING "Build miniaudio without runtime library/framework loading (Apple app-bundle path)" OFF)

set(CTH_DEV_DSP "/dev/dsp" CACHE STRING "Default OSS DSP device")
set(CTH_DEV_MIXER "/dev/mixer" CACHE STRING "Default OSS mixer device")
set(CTH_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/cthughanix" CACHE PATH
    "Installed runtime data directory")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(endian.h HAVE_ENDIAN_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(getopt.h HAVE_GETOPT_H)
check_include_file(linux/soundcard.h HAVE_LINUX_SOUNDCARD_H)
check_include_file(ndir.h HAVE_NDIR_H)
check_include_file(sys/dir.h HAVE_SYS_DIR_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_file(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/wait.h HAVE_SYS_WAIT_H)
check_include_file(unistd.h HAVE_UNISTD_H)

check_function_exists(gethostname HAVE_GETHOSTNAME)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(select HAVE_SELECT)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtol HAVE_STRTOL)
check_function_exists(vprintf HAVE_VPRINTF)

test_big_endian(CTH_WORDS_BIGENDIAN)
set(TIME_WITH_SYS_TIME 1)
set(WITH_SAVER 0)

find_package(Threads REQUIRED)
if(CTH_ENABLE_DSP AND (HAVE_LINUX_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H))
    set(WITH_DSP 1)
else()
    set(WITH_DSP 0)
endif()

if(CTH_ENABLE_MIXER AND (HAVE_LINUX_SOUNDCARD_H OR HAVE_SYS_SOUNDCARD_H))
    set(WITH_MIXER 1)
else()
    set(WITH_MIXER 0)
endif()

find_package(PkgConfig QUIET)
if(CTH_ENABLE_PULSE AND PkgConfig_FOUND)
    pkg_check_modules(PULSE libpulse-simple)
endif()
if(CTH_ENABLE_PULSE AND PULSE_FOUND)
    set(WITH_PULSE 1)
else()
    set(WITH_PULSE 0)
endif()

if(CTH_ENABLE_MINIMP3)
    set(WITH_MINIMP3 1)
else()
    set(WITH_MINIMP3 0)
endif()

if(CTH_ENABLE_MINIAUDIO)
    set(WITH_MINIAUDIO 1)
else()
    set(WITH_MINIAUDIO 0)
endif()

if(WITH_MINIAUDIO)
    if(CTH_MINIAUDIO_NO_RUNTIME_LINKING AND NOT APPLE)
        message(FATAL_ERROR "CTH_MINIAUDIO_NO_RUNTIME_LINKING is currently supported only for Apple builds")
    endif()

    add_library(cthugha_miniaudio INTERFACE)
    target_include_directories(cthugha_miniaudio INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
    )
    target_link_libraries(cthugha_miniaudio INTERFACE
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )

    if(CTH_MINIAUDIO_NO_RUNTIME_LINKING)
        target_compile_definitions(cthugha_miniaudio INTERFACE
            MA_NO_RUNTIME_LINKING
        )

        find_library(CTH_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
        find_library(CTH_COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
        find_library(CTH_AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
        target_link_libraries(cthugha_miniaudio INTERFACE
            ${CTH_COREFOUNDATION_FRAMEWORK}
            ${CTH_COREAUDIO_FRAMEWORK}
            ${CTH_AUDIOTOOLBOX_FRAMEWORK}
        )
    endif()
endif()

set(WITH_X11 0)
if(CTH_BUILD_X11)
    set(WITH_X11 1)
    find_package(X11 REQUIRED)
    find_package(ZLIB REQUIRED)

    foreach(required_x11_lib X11_Xext_LIB X11_Xt_LIB X11_Xaw_LIB X11_Xmu_LIB)
        if(NOT ${required_x11_lib})
            message(FATAL_ERROR "Required X11 library was not found: ${required_x11_lib}")
        endif()
    endforeach()

endif()

if(CTH_WORDS_BIGENDIAN)
    set(WORDS_BIGENDIAN 1)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

add_subdirectory(src)

if(CTH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CTH_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_subdirectory(tests/benchmarks)
endif()

install(DIRECTORY resources/img/ DESTINATION ${CTH_DATA_DIR}/img)
install(DIRECTORY resources/map/ DESTINATION ${CTH_DATA_DIR}/map
        FILES_MATCHING PATTERN "*.map")
install(DIRECTORY resources/obj/ DESTINATION ${CTH_DATA_DIR}/obj
        FILES_MATCHING PATTERN "*.obj")

message(STATUS "")
message(STATUS "CthughaNix modern CMake summary")
message(STATUS "  X11 frontend       : ${WITH_X11}")
message(STATUS "  SDL3 frontend      : ${CTH_BUILD_SDL3}")
message(STATUS "  Pulse output       : ${WITH_PULSE}")
message(STATUS "  OSS DSP input/output: ${WITH_DSP} (${CTH_DEV_DSP})")
message(STATUS "  OSS mixer          : ${WITH_MIXER} (${CTH_DEV_MIXER})")
message(STATUS "  minimp3 decoder    : ${WITH_MINIMP3}")
message(STATUS "  miniaudio devices  : ${WITH_MINIAUDIO}")
message(STATUS "  miniaudio no runtime linking: ${CTH_MINIAUDIO_NO_RUNTIME_LINKING}")
message(STATUS "  audio device tests : ${CTH_RUN_AUDIO_DEVICE_TESTS}")
message(STATUS "  Benchmarks         : ${CTH_BUILD_BENCHMARKS}")
message(STATUS "  Runtime data dir   : ${CTH_DATA_DIR}")
