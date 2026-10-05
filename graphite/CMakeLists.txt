cmake_minimum_required(VERSION 3.16)
project(graphite)

include(FetchContent)
set(SOURCES source/pch.cpp source/core.cpp)

if(GRAPHITE_PLATFORM_BACKEND STREQUAL "glfw")
  list(APPEND SOURCES source/glfw_platform.cpp)
endif()

if(GRAPHITE_GRAPHICS_BACKEND STREQUAL "vulkan")
  list(
    APPEND
    SOURCES
    source/vk_core.cpp
    source/vk_execution.cpp
    source/vk_resources.cpp
    source/vk_shader.cpp
    source/vk_swap_chain.cpp)
endif()

if(GRAPHITE_SHADER_COMPILATION_BACKEND STREQUAL "slang")
  list(APPEND SOURCES source/slang_shader.cpp)
endif()
if(GRAPHITE_SHADER_REFLECTION_BACKEND STREQUAL "spirv-reflect")
  list(APPEND SOURCES source/spirv_shader.cpp)
endif()

add_library(graphite STATIC ${SOURCES})
target_compile_definitions(graphite PUBLIC GRAPH_VERSION=\"v0.1.x\")
target_include_directories(graphite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_precompile_headers(graphite PRIVATE
                          ${CMAKE_CURRENT_SOURCE_DIR}/source/pch.hpp)

if(GRAPHITE_PLATFORM_BACKEND STREQUAL "glfw")

  find_package(glfw3 QUIET)

  if(NOT TARGET glfw)
    message(STATUS "GRAPHITE - GLFW not found. Fetching source...")
    FetchContent_Declare(
      glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.4
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE)

    set(GLFW_BUILD_DOCS
        OFF
        CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES
        OFF
        CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS
        OFF
        CACHE BOOL "" FORCE)
    set(GLFW_INSTALL
        OFF
        CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw)
    tkit_configure_compile_link_options(glfw NO_EXCEPTIONS NO_RTTI)

  else()
    message(STATUS "GRAPHITE - GLFW found")
  endif()

  target_compile_definitions(graphite PUBLIC GRAPH_HAS_PLATFORM_BACKEND
                                             GRAPH_PLATFORM_BACKEND_GLFW)
  target_link_libraries(graphite PRIVATE glfw)
endif()

if(GRAPHITE_GRAPHICS_BACKEND STREQUAL "vulkan")

  FetchContent_Declare(
    vulkit
    GIT_REPOSITORY https://github.com/ismawno/vulkit.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(vulkit)

  target_link_libraries(
    graphite
    PUBLIC toolkit
    PRIVATE vulkit)

  target_compile_definitions(graphite PUBLIC GRAPH_HAS_RENDER_BACKEND
                                             GRAPH_RENDER_BACKEND_VULKAN)

  tkit_default_configure(graphite)
endif()

if(GRAPHITE_SHADER_COMPILATION_BACKEND STREQUAL "slang")
  if(DEFINED ENV{VCPKG_ROOT})
    include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
  endif()
  find_package(slang QUIET)
  find_path(SLANG_INCLUDE_DIR slang.h)
  find_library(SLANG_LIB slang)
  if(slang_FOUND)
    message(STATUS "GRAPHITE - Slang found")
    target_link_libraries(graphite PRIVATE slang::slang)
  elseif(SLANG_INCLUDE_DIR AND SLANG_LIB)
    message(STATUS "GRAPHITE - Slang library found")
    target_include_directories(graphite PRIVATE ${SLANG_INCLUDE_DIR})
    target_link_libraries(graphite PRIVATE ${SLANG_LIB})
  else()
    message(STATUS "GRAPHITE - Slang not found... fetching source")

    FetchContent_Declare(
      slang
      GIT_REPOSITORY https://github.com/shader-slang/slang.git
      GIT_TAG v2026.3.1
      GIT_SHALLOW TRUE
      GIT_SUBMODULES_RECURSE TRUE
      GIT_PROGRESS TRUE)

    set(SLANG_ENABLE_DXIL
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_GFX
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_SLANG_GLSLANG
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_SLANGC
        ON
        CACHE BOOL "")
    set(SLANG_ENABLE_SLANGD
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_SLANGI
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_TESTS
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_CUDA
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_EXAMPLES
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_RELEASE_DEBUG_INFO
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_SPLIT_DEBUG_INFO
        OFF
        CACHE BOOL "")
    set(SLANG_ENABLE_SLANG_RHI
        OFF
        CACHE BOOL "")
    set(SLANG_LIB_TYPE
        "STATIC"
        CACHE STRING "")

    FetchContent_MakeAvailable(slang)
    tkit_configure_compile_link_options(slang NO_EXCEPTIONS NO_RTTI)

    target_link_libraries(graphite PRIVATE slang)
  endif()
  target_compile_definitions(
    graphite PUBLIC GRAPH_HAS_SHADER_COMPILATION_BACKEND
                    GRAPH_SHADER_COMPILATION_BACKEND_SLANG)
endif()

if(GRAPHITE_SHADER_REFLECTION_BACKEND STREQUAL "spirv-reflect")

  FetchContent_Declare(
    spirv-reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  set(SPIRV_REFLECT_EXECUTABLE
      OFF
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_STATIC_LIB
      ON
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_SHARED_LIB
      OFF
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_ENABLE_ASSERTS
      OFF
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_ENABLE_ASAN
      OFF
      CACHE BOOL "" FORCE)
  set(SPIRV_REFLECT_INSTALL
      OFF
      CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(spirv-reflect)
  tkit_configure_compile_link_options(spirv-reflect-static NO_EXCEPTIONS
                                      NO_RTTI)

  target_compile_definitions(
    graphite PUBLIC GRAPH_HAS_SHADER_REFLECTION_BACKEND
                    GRAPH_SHADER_REFLECTION_BACKEND_SLANG)
  target_link_libraries(graphite PRIVATE spirv-reflect-static)

endif()
