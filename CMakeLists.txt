cmake_minimum_required(VERSION 3.1.0)
project(SlimeGame VERSION 0.1.0)

#include(CTest)
#enable_testing()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

link_directories("lib/Debug")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} raylib)
target_include_directories(${PROJECT_NAME} PRIVATE include src)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)

#include(CMakePrintHelpers)
#cmake_print_variables(PROJECT_SOURCE_DIR)
#cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)