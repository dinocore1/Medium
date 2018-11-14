cmake_minimum_required(VERSION 3.1)
include(cmake/msvc_precompiled.cmake)
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(ExternalProject)

project(medium)

externalproject_add(external_baseline
  URL https://github.com/dinocore1/Baseline/archive/master.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/baseline
  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

file(GLOB Medium_SRCS src/*.cc)
ADD_MSVC_PRECOMPILED_HEADER("src/stdafx.h" "stdafx.cc" Medium_SRCS)

add_library(medium ${Medium_SRCS})
target_include_directories(medium PUBLIC src src/include)
add_dependencies(medium external_baseline)

set(UnitTest_SRCS "")
ADD_MSVC_PRECOMPILED_HEADER("tests/catch.hpp" "catch.cpp" UnitTest_SRCS)

add_executable(test1 ${UnitTest_SRCS} tests/test1.cc)
target_link_libraries(test1 medium baseline)