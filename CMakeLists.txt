cmake_minimum_required(VERSION 3.22)
project (bass CXX)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.hpp include/common/*.hpp  include/core/*.hpp include/frontend/*.hpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/core/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/core/version.h
                   -P ${CMAKE_SOURCE_DIR}/GenerateVersion.cmake
  )



add_dependencies(${PROJECT_NAME} version)

if(UNIX)
target_compile_options(${PROJECT_NAME} PRIVATE -g -std=c++20 -Werror -Wextra -Wall -Wconversion -fconstexpr-depth=9999999 -Wno-error=sign-conversion)
elseif(WIN32)
target_compile_options(${PROJECT_NAME} PRIVATE /g /utf-8 /std:c++20 /SUBSYSTEM:WINDOWS)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE include include/core lib/ lib/imgui lib/imgui/backends)


if(NOT SDL3_FOUND)
include(FetchContent)
    set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
    FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    message(STATUS "Fetching SDL3 (FetchContent)")
    FetchContent_MakeAvailable(SDL)
    set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
endif()


include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
message(STATUS "Fetching ImGui (FetchContent)")

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)

  
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
  )

  target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src/include
  )
endif()


include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(spdlog)


include(FetchContent)

FetchContent_Declare(
    capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
    GIT_TAG next   
)

FetchContent_MakeAvailable(capstone)


target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 imgui spdlog capstone)