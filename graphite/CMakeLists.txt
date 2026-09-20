cmake_minimum_required(VERSION 3.16)
project(graphite)

include(FetchContent)
set(SOURCES source/pch.cpp source/handle.cpp)

if(GRAPHITE_GRAPHICS_BACKEND STREQUAL "vulkan")
  list(APPEND SOURCES source/vk_core.cpp)
  if(GRAPHITE_PLATFORM_BACKEND STREQUAL "glfw")
    list(APPEND SOURCES source/vk_core.cpp source/vk_glfw_platform.cpp
         source/vk_surface.cpp)
  endif()
endif()

add_library(graphite STATIC ${SOURCES})

if(GRAPHITE_GRAPHICS_BACKEND STREQUAL "vulkan")

  target_compile_definitions(graphite PUBLIC GRAPH_VERSION=\"v0.1.x\")

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

    target_compile_definitions(graphite PRIVATE GRAPH_HAS_PLATFORM_BACKEND
                                                GRAPH_PLATFORM_BACKEND_GLFW)
    target_link_libraries(graphite PRIVATE glfw)
  endif()

  FetchContent_Declare(
    vulkit
    GIT_REPOSITORY https://github.com/ismawno/vulkit.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(vulkit)

  target_include_directories(graphite
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(
    graphite
    PUBLIC toolkit
    PRIVATE vulkit)

  target_precompile_headers(graphite PRIVATE
                            ${CMAKE_CURRENT_SOURCE_DIR}/source/pch.hpp)

  tkit_default_configure(graphite)
endif()
