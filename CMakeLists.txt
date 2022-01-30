cmake_minimum_required(VERSION 3.1.0)
project(SlimeGame VERSION 0.1.0)

#include(CTest)
#enable_testing()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

#option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
#if (${FORCE_COLORED_OUTPUT})
#    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#        add_compile_options (-fdiagnostics-color=always)
#    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#        add_compile_options(-fcolor-diagnostics)
#    endif ()
#endif ()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/jail_enet.cpp
    src/jail_imgui.cpp
    src/jail_win32_console.cpp
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _DEBUG
    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
)
target_include_directories(${PROJECT_NAME} PRIVATE include src)
target_link_directories(${PROJECT_NAME} PRIVATE "lib/Debug")
target_link_libraries(${PROJECT_NAME}
    raylib.lib
    ws2_32.lib
    winmm.lib
)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)

#include(CMakePrintHelpers)
#cmake_print_variables(PROJECT_SOURCE_DIR)
#cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)