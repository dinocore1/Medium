cmake_minimum_required(VERSION 2.95)
include(cmake/msvc_precompiled.cmake)
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(medium)

file(GLOB Medium_SRCS src/*.cc)
ADD_MSVC_PRECOMPILED_HEADER("src/stdafx.h" "stdafx.cc" Medium_SRCS)

add_library(medium ${Medium_SRCS})
target_include_directories(medium PUBLIC src src/include)

set(UnitTest_SRCS "")
ADD_MSVC_PRECOMPILED_HEADER("tests/catch.hpp" "catch.cpp" UnitTest_SRCS)

add_executable(test1 ${UnitTest_SRCS} tests/test1.cc)
target_link_libraries(test1 medium)