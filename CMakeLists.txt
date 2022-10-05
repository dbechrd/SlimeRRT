cmake_minimum_required(VERSION 3.1.0)
project(SlimeGame VERSION 0.1.0)

#include(CTest)
#enable_testing()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _DEBUG
        _CONSOLE
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #message("Clang detected, setting flags")
    #add_compile_options(-fcolor-diagnostics)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-security")
    target_compile_options(${PROJECT_NAME} PRIVATE
        # All warnings, warnings as errors, be pedantic.
        -Wall
        -Wextra
        -Werror
        -Wpedantic

        # Warnings I might want to turn back on and fix some day
        -Wno-non-virtual-dtor
        -Wno-switch-enum
        -Wno-switch
        -Wno-implicit-fallthrough
        -Wno-exit-time-destructors
        -Wno-unused-macros
        -Wno-delete-non-abstract-non-virtual-dtor # I have _NO_ fucking clue what this means

        # Disable warnings
        -Wno-format-security
        -Wno-deprecated-declarations
        -Wno-old-style-cast
        -Wno-cast-align
        -Wno-zero-as-null-pointer-constant
        -Wno-global-constructors
        -Wno-missing-prototypes
        -Wno-covered-switch-default
        -Wno-implicit-int-conversion
        -Wno-implicit-int-float-conversion
        -Wno-cast-qual
        -Wno-shorten-64-to-32
        -Wno-reserved-identifier
        -Wno-float-equal
        -Wno-shadow-field-in-constructor
        -Wno-float-conversion
        -Wno-documentation-unknown-command
        -Wno-newline-eof
        -Wno-extra-semi-stmt
        -Wno-shadow
        -Wno-sign-conversion
        -Wno-documentation
        -Wno-undef
        -Wno-missing-noreturn
        -Wno-format-nonliteral
        -Wno-double-promotion
        -Wno-malformed-warning-check
        #-Wno-gnu-zero-variadic-macro-arguments
        # Disable warnings about C++98 incompatibility. We're using C++20...
        #-Wno-c++98-compat
        -Wno-c++98-compat-pedantic
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    #add_compile_options (-fdiagnostics-color=always)
endif ()

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