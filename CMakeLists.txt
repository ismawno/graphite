cmake_minimum_required(VERSION 3.16...3.31)
project(graphite-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE
    "Debug"
    CACHE STRING "")

set(GRAPHITE_GRAPHICS_BACKEND
    "vulkan"
    CACHE STRING "")
set(GRAPHITE_PLATFORM_BACKEND
    "none" # glfw
    CACHE STRING "")
set(GRAPHITE_SHADER_COMPILATION_BACKEND
    "none" # slang
    CACHE STRING "")
set(GRAPHITE_SHADER_REFLECTION_BACKEND
    "none" # spirv-reflect
    CACHE STRING "")

add_subdirectory(graphite)
