cmake_minimum_required(VERSION 3.1)
include(cmake/msvc_precompiled.cmake)

set (CMAKE_CXX_STANDARD 11)

project(medium)

#file(GLOB Medium_SRCS src/*.cc)
set(Medium_SRCS
    src/DatablockDB.cc
    src/DataSplitterOutputStream.cc
    src/FileSystem.cc
    src/InodeDB.cc
    src/main.cc
    src/RabinKarpHash.cc
)

include_directories(src ${CMAKE_CURRENT_BINARY_DIR}/include ${FLINT_BUILDROOT}/include)
link_directories(${FLINT_BUILDROOT}/lib)

add_executable(medium ${Medium_SRCS} )
target_link_libraries(medium baseline fuse db_cxx pthread)
target_compile_definitions(medium PRIVATE _FILE_OFFSET_BITS=64)