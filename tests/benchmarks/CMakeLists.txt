add_executable(audio_pipeline_bench
    AudioPipelineBench.cc
    ${CMAKE_SOURCE_DIR}/src/AudioInternal.cc
    ${CMAKE_SOURCE_DIR}/src/AudioOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioPulseOutput.cc
    ${CMAKE_SOURCE_DIR}/src/AudioDSPOutput.cc
    ${CMAKE_SOURCE_DIR}/src/DecodedAudioHistory.cc
    ${CMAKE_SOURCE_DIR}/src/PcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/DspPcmSource.cc
    ${CMAKE_SOURCE_DIR}/src/AudioAnalyzer.cc
    ${CMAKE_SOURCE_DIR}/src/AudioFrame.cc
    ${CMAKE_SOURCE_DIR}/src/AudioProcessor.cc
    ${CMAKE_SOURCE_DIR}/src/AudioVisualBridge.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
    ${CMAKE_SOURCE_DIR}/src/misc.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
)

target_compile_definitions(audio_pipeline_bench PRIVATE
    CTH_AUDIO_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/audio"
    CTH_AUDIO_FRAME_NO_RUNTIME
    CTH_AUDIO_FRAME_TEST_OVERRIDE
    CTH_AUDIO_VISUAL_BRIDGE_NO_AUTOCHANGER
    CTH_LIBDIR="${CTH_DATA_DIR}"
)

target_include_directories(audio_pipeline_bench PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${PULSE_INCLUDE_DIRS}
)

target_compile_options(audio_pipeline_bench PRIVATE ${PULSE_CFLAGS_OTHER})

target_link_libraries(audio_pipeline_bench PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    Threads::Threads
    m
)

if(WITH_PULSE)
    target_link_libraries(audio_pipeline_bench PRIVATE ${PULSE_LIBRARIES})
    target_link_directories(audio_pipeline_bench PRIVATE ${PULSE_LIBRARY_DIRS})
endif()

add_executable(visual_effects_bench
    VideoEffectsBench.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControl.cc
    ${CMAKE_SOURCE_DIR}/src/EffectControlPolicy.cc
    ${CMAKE_SOURCE_DIR}/src/EffectPresetCatalog.cc
    ${CMAKE_SOURCE_DIR}/src/EffectChoice.cc
    ${CMAKE_SOURCE_DIR}/src/CthughaBuffer.cc
    ${CMAKE_SOURCE_DIR}/src/Flame.cc
    ${CMAKE_SOURCE_DIR}/src/flames.cc
    ${CMAKE_SOURCE_DIR}/src/imath.cc
    ${CMAKE_SOURCE_DIR}/src/misc.cc
    ${CMAKE_SOURCE_DIR}/src/Option.cc
    ${CMAKE_SOURCE_DIR}/src/OptionInt.cc
    ${CMAKE_SOURCE_DIR}/src/Translate.cc
    ${CMAKE_SOURCE_DIR}/src/TranslateGenerator.cc
    ${CMAKE_SOURCE_DIR}/src/TranslationOptions.cc
    ${CMAKE_SOURCE_DIR}/src/VideoFilterchain.cc
)

target_compile_definitions(visual_effects_bench PRIVATE
    CTH_LIBDIR="${CTH_DATA_DIR}"
    CTH_VISUAL_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/visual"
)

target_include_directories(visual_effects_bench PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(visual_effects_bench PRIVATE
    benchmark::benchmark
    Threads::Threads
    m
)

add_custom_target(visual_effects_benchmark_report
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_visual_effects_benchmarks.py
        --build-dir ${CMAKE_BINARY_DIR}
    DEPENDS visual_effects_bench
    USES_TERMINAL
    VERBATIM
)

add_custom_target(audio_pipeline_benchmark_report
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_audio_pipeline_benchmarks.py
        --build-dir ${CMAKE_BINARY_DIR}
    DEPENDS audio_pipeline_bench
    USES_TERMINAL
    VERBATIM
)
