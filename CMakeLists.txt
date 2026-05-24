cmake_minimum_required(VERSION 3.22)

project(libbgt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BGT_BUILD_EXAMPLES "Build libbgt examples" ON)
option(BGT_BUILD_TESTS "Build libbgt tests" OFF)
option(BGT_BUILD_SHARED "Build libbgt as a shared library" OFF)
option(BGT_USE_SYSTEM_SDL "Use system-installed SDL3 and SDL3_ttf packages" OFF)

set(BGT_LIBRARY_TYPE STATIC)
if(BGT_BUILD_SHARED)
    set(BGT_LIBRARY_TYPE SHARED)
endif()

if(BGT_USE_SYSTEM_SDL)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_ttf CONFIG REQUIRED)
else()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL/CMakeLists.txt")
        message(FATAL_ERROR "third_party/SDL is missing. Run: git submodule update --init --recursive")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL_ttf/CMakeLists.txt")
        message(FATAL_ERROR "third_party/SDL_ttf is missing. Run: git submodule update --init --recursive")
    endif()

    # set the output directory for built objects.
    # This makes sure that the dynamic library goes into the build directory automatically.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

    set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

    add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(third_party/SDL_ttf EXCLUDE_FROM_ALL)
endif()

add_library(bgt ${BGT_LIBRARY_TYPE} src/bgt.cpp)
add_library(libbgt::bgt ALIAS bgt)

target_compile_features(bgt PUBLIC cxx_std_20)
set_target_properties(bgt PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(bgt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(bgt PRIVATE BGT_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_link_libraries(bgt PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)

if(MSVC)
    target_compile_options(bgt PRIVATE /W4 /utf-8)
else()
    target_compile_options(bgt PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BGT_BUILD_EXAMPLES)
    add_executable(bgt_hello examples/01_hello.cpp)
    target_link_libraries(bgt_hello PRIVATE libbgt::bgt)
    set_target_properties(bgt_hello PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(bgt_hello PRIVATE /utf-8)
    endif()
endif()

if(BGT_BUILD_TESTS)
    enable_testing()
endif()
