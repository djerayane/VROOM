cmake_minimum_required(VERSION 3.24)

project(vroom_packager LANGUAGES CXX)

add_executable(vroom_packager main.cpp)

target_include_directories(vroom_packager PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)

target_compile_features(vroom_packager PRIVATE cxx_std_20)

