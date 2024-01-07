cmake_minimum_required(VERSION 3.5)
project(AnimationPlayer)

set(CMAKE_PREFIX_PATH "/home/will/.local/mambaforge/envs/cxx")
include_directories(/home/will/.local/mambaforge/envs/cxx/include)
link_directories(/home/will/.local/mambaforge/envs/cxx/lib)
find_package(glfw3 REQUIRED)
find_package(glad  REQUIRED)
find_package(glm   REQUIRED)
find_package(Boost REQUIRED)

add_definitions(-DGLFW_INCLUDE_NONE)
#file(COPY base.frag base.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} main.cpp base.frag base.glsl)
target_link_libraries(${PROJECT_NAME} glfw glad)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_library(module SHARED module.cpp)
