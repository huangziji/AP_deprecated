cmake_minimum_required(VERSION 3.15)
project(AnimationPlayer)

include(../build/conan_toolchain.cmake)
include_directories(${CMAKE_INCLUDE_PATH})
find_package(Bullet REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/cxx/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)
find_package(Boost REQUIRED)

link_directories(
    ${CMAKE_LIBRARY_PATH}
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
    ${CMAKE_PREFIX_PATH}/lib
)
target_link_libraries(
    AnimationPlayer
    glfw glad dl
    # Module
    Static
)

add_library(Static
    Geometry.cpp
)
# target_include_directories(
# )
# target_link_directories(
# )
# target_link_libraries(
# )

add_library(
    Module SHARED
    module.cpp
    ikrig.cpp common.cpp common.h
)
target_include_directories(
    Module PRIVATE
    ${CMAKE_PREFIX_PATH}/include
)
target_link_directories(
    Module PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
    Module PRIVATE
    Static
)
