cmake_minimum_required(VERSION 3.21)

project(
    JM_Poison
    VERSION 0.1.0
    DESCRIPTION "Configurable poison dose progression for Skyrim SE 1.5.97"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(CommonLibSSE CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED src/main.cpp)
target_commonlibsse_properties(${PROJECT_NAME})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

target_include_directories(${PROJECT_NAME} PRIVATE src)

install(TARGETS ${PROJECT_NAME} DESTINATION SKSE/Plugins)