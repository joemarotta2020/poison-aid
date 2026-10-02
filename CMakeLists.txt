cmake_minimum_required(VERSION 3.21)

project(
    JM_Poison
    VERSION 0.1.0
    DESCRIPTION "Configurable poison dose progression for Skyrim SE 1.5.97"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(FetchContent)

# Build CommonLibSSE-NG from source, SE-only, with Xbyak support.
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(CommonLibSSE)
include("${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake")

add_commonlibsse_plugin(JM_Poison
    NAME "JM_Poison"
    VERSION ${PROJECT_VERSION}
    COMPATIBLE_RUNTIMES "1.5.97"
    SOURCES
        src/main.cpp
)

target_precompile_headers(JM_Poison PRIVATE src/PCH.h)
target_include_directories(JM_Poison PRIVATE src)
target_compile_definitions(JM_Poison PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

install(TARGETS JM_Poison DESTINATION SKSE/Plugins)
