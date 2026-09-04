cmake_minimum_required(VERSION 3.22)

project(libbgt VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Treat all project and vendored dependency sources as UTF-8 when using the
# MSVC command-line interface. This also prevents C4819 warnings from sources
# containing characters that are not representable in the active code page.
if(MSVC)
    add_compile_options(/utf-8)
endif()

option(BGT_BUILD_EXAMPLES "Build libbgt examples" ON)
option(BGT_BUILD_TESTS "Build libbgt tests" OFF)
option(BGT_BUILD_SHARED "Build libbgt as a shared library" OFF)
option(BGT_BUILD_VENDORED "Merge libbgt and vendored dependencies into one MSVC static library" OFF)
option(BGT_USE_SYSTEM_SDL "Use system-installed SDL3 and SDL3_ttf packages" OFF)

if(BGT_BUILD_VENDORED AND (NOT MSVC OR BGT_BUILD_SHARED OR BGT_USE_SYSTEM_SDL))
    message(FATAL_ERROR
        "BGT_BUILD_VENDORED requires MSVC, bundled SDL, and BGT_BUILD_SHARED=OFF")
endif()

set(BGT_LIBRARY_TYPE STATIC)
if(BGT_BUILD_SHARED)
    set(BGT_LIBRARY_TYPE SHARED)
endif()

if(BGT_USE_SYSTEM_SDL)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_ttf CONFIG REQUIRED)

    if(NOT TARGET SDL3::SDL3-static OR NOT TARGET SDL3_ttf::SDL3_ttf-static)
        message(FATAL_ERROR
            "BGT_USE_SYSTEM_SDL requires SDL3 and SDL3_ttf packages with static targets")
    endif()
else()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/CMakeLists.txt")
        message(FATAL_ERROR "third_party/SDL is missing. Run: git submodule update --init --recursive")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL_ttf/CMakeLists.txt")
        message(FATAL_ERROR "third_party/SDL_ttf is missing. Run: git submodule update --init --recursive")
    endif()

    # Build the bundled dependencies only as static libraries. Besides making the
    # examples self-contained, the explicit settings prevent SDL3::SDL3 from
    # selecting SDL's shared target when both variants are available.
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

    add_subdirectory(third_party/SDL SYSTEM EXCLUDE_FROM_ALL)
    add_subdirectory(third_party/SDL_ttf SYSTEM EXCLUDE_FROM_ALL)
endif()

add_library(bgt ${BGT_LIBRARY_TYPE} src/bgt.cpp)
add_library(libbgt::bgt ALIAS bgt)

target_compile_features(bgt PUBLIC cxx_std_20)
set_target_properties(bgt PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(
    bgt
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if(BGT_BUILD_SHARED)
    target_link_libraries(bgt PRIVATE SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static)
else()
    target_compile_definitions(bgt PUBLIC BGT_STATIC_DEFINE)
    target_link_libraries(bgt PUBLIC SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static)
endif()

set_target_properties(bgt PROPERTIES
    DEBUG_POSTFIX d
    WINDOWS_EXPORT_ALL_SYMBOLS "${BGT_BUILD_SHARED}")

if(BGT_BUILD_VENDORED)
    set(BGT_VENDORED_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/bgt_vendored.lib")
    add_custom_command(
        OUTPUT "${BGT_VENDORED_LIBRARY}"
        COMMAND "${CMAKE_AR}" /NOLOGO "/OUT:${BGT_VENDORED_LIBRARY}"
                "$<TARGET_FILE:bgt>"
                "$<TARGET_FILE:SDL3-static>"
                "$<TARGET_FILE:SDL3_ttf-static>"
        DEPENDS bgt SDL3-static SDL3_ttf-static
        COMMENT "Merging libbgt and vendored dependencies"
        VERBATIM)
    add_custom_target(bgt_vendored ALL DEPENDS "${BGT_VENDORED_LIBRARY}")
endif()

if(MSVC)
    target_compile_options(bgt PRIVATE /W4)
else()
    target_compile_options(bgt PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BGT_BUILD_EXAMPLES)
    function(bgt_add_example target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE libbgt::bgt)
        set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

    endfunction()

    bgt_add_example(bgt_hello examples/01_hello.cpp)
    bgt_add_example(bgt_shapes examples/02_shapes.cpp)
    bgt_add_example(bgt_text examples/03_text.cpp)
    bgt_add_example(bgt_input examples/04_input.cpp)
    bgt_add_example(bgt_transparency examples/05_transparency.cpp)
    bgt_add_example(bgt_sudoku examples/06_sudoku.cpp)
    bgt_add_example(bgt_hanoi examples/07_hanoi.cpp)
    bgt_add_example(bgt_api_tour examples/08_api_tour.cpp)
    bgt_add_example(bgt_storage examples/12_storage.cpp)

    add_custom_command(
        TARGET bgt_sudoku
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/06_sudoku_puzzle.txt"
                "$<TARGET_FILE_DIR:bgt_sudoku>/06_sudoku_puzzle.txt"
        VERBATIM)
endif()

if(BGT_BUILD_TESTS)
    enable_testing()

    add_executable(bgt_test_storage tests/test_storage.cpp)
    target_link_libraries(bgt_test_storage PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_storage PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    add_test(NAME bgt_test_storage COMMAND bgt_test_storage)
endif()

if(BGT_BUILD_VENDORED)
    install(FILES "${BGT_VENDORED_LIBRARY}"
        TYPE LIB)
else()
    install(
        TARGETS bgt
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/bgt.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME
    "libbgt-${PROJECT_VERSION}-msvc-x64")
include(CPack)
