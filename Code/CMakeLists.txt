cmake_minimum_required(VERSION 3.15)
project(AnimationPlayer)

include(../build/conan_toolchain.cmake)
include_directories(${CMAKE_INCLUDE_PATH})
find_package(PhysX REQUIRED)
# find_package(Bullet REQUIRED)

set(CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/cxx/)
add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)
find_package(Boost REQUIRED)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    base.glsl
    base.frag
    line.glsl
    shadowmap.glsl
)
target_link_directories(
    ${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_PREFIX_PATH}/lib
    ${CMAKE_LIBRARY_PATH}
)
target_link_libraries(${PROJECT_NAME} glfw glad dl pthread
    # BulletSoftBody Bullet3Common BulletDynamics BulletCollision LinearMath
)

add_library(Module SHARED module.cpp ikrig.cpp common.cpp common.h)
target_include_directories(Module PUBLIC ${CMAKE_PREFIX_PATH}/include)
target_link_directories(
    Module PRIVATE
    ${CMAKE_LIBRARY_PATH}
)
target_link_libraries(
    Module
    # PhysXPvdSDK
    # PhysXVehicle
    # PhysXCharacterKinematic
    # PhysXCooking
    PhysX
    PhysXExtensions
    PhysXCommon
    PhysXFoundation
)
