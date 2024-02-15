cmake_minimum_required(VERSION 3.15)
project(AnimationPlayer)

include(../build/conan_toolchain.cmake)
include_directories(${CMAKE_INCLUDE_PATH})
find_package(Bullet REQUIRED)
find_package(recastnavigation REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/cxx/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)
find_package(Boost REQUIRED)

include_directories(
    ${CMAKE_PREFIX_PATH}/include
)
link_directories(
    ${CMAKE_LIBRARY_PATH}
    ${CMAKE_PREFIX_PATH}/lib
)
link_libraries(
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
)

add_executable(
    AnimationPlayer
    main.cpp
    base.glsl
    base.frag
    line.glsl
    shadowmap.glsl
)
target_link_directories(
    AnimationPlayer PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    AnimationPlayer
    glfw glad dl
    Static
    # Module
)

add_library(Static
    Geometry.cpp
    common.cpp
    common.h
)

add_library(
    Module SHARED
    module.cpp
    ikrig.cpp
)
target_link_directories(
    Module PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    Module PRIVATE
    Static
)
