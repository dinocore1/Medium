cmake_minimum_required(VERSION 2.95)

project(medium)

file(GLOB Medium_SRCS src/*.cc)

add_library(medium ${Medium_SRCS})
target_include_directories(medium PUBLIC src src/include)


add_executable(test1 tests/test1.cc)
target_link_libraries(test1 medium)