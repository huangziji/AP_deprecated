cmake_minimum_required(VERSION 3.15)
project(AnimationPlayer)

include(../build/conan_toolchain.cmake)
include_directories(${CMAKE_INCLUDE_PATH})
find_package(PhysX REQUIRED)
#find_package(imgui REQUIRED)

set(CMAKE_PREFIX_PATH /home/will/.local/micromamba/envs/cxx)
link_directories(${CMAKE_PREFIX_PATH}/lib)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)
find_package(Boost REQUIRED)
#find_package(Bullet REQUIRED)
#find_package(sdl2 REQUIRED)
find_package(OpenAL REQUIRED)
add_definitions(-DGLFW_INCLUDE_NONE)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} glfw glad openal Common)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_library(Common SHARED common.cpp Geometry.cpp
    stb_image.h base.glsl base.frag line.glsl splat.glsl common.glsl shadowmap.glsl)
add_library(Module SHARED module.cpp)
target_link_libraries(Module glfw glad Common)
target_link_libraries(Common glfw glad)
