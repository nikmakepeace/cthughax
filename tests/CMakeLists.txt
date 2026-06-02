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
