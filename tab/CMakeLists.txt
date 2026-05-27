set(CTH_TAB_DATA
    bighalfwheel.cmd
    cmdRead.cmd
    downspiral.cmd
    gentable.cmd
    hurricane.cmd
    randswirls.cmd
    rotate.cmd
    smoke.cmd
    space-fast.cmd
    space-slow.cmd
    yin-yang.cmd
)

add_executable(cmd_huricn cmd_huricn.c imath.c)
add_executable(cmd_smoke cmd_smoke.c imath.c)
add_executable(cmd_space cmd_space.c imath.c)
add_executable(cmd_gentable cmd_gentable.c imath.c)
add_executable(cmd_bighalfwheel cmd_bighalfwheel.c imath.c)
add_executable(cmd_downspiral cmd_downspiral.c imath.c)
add_executable(cmd_randswirls cmd_randswirls.c)
add_executable(cmdRead cmdRead.cc)

foreach(tool
        cmd_huricn
        cmd_smoke
        cmd_space
        cmd_gentable
        cmd_bighalfwheel
        cmd_downspiral
        cmd_randswirls
        cmdRead)
    target_include_directories(${tool} PRIVATE ${CMAKE_BINARY_DIR}/src)
    target_link_libraries(${tool} PRIVATE m)
    install(TARGETS ${tool} RUNTIME DESTINATION ${CTH_DATA_DIR}/tab)
endforeach()

install(FILES ${CTH_TAB_DATA} DESTINATION ${CTH_DATA_DIR}/tab)
