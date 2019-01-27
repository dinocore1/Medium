cmake_minimum_required(VERSION 3.1)
include(cmake/msvc_precompiled.cmake)

set (CMAKE_CXX_STANDARD 11)

project(medium)

#file(GLOB Medium_SRCS src/*.cc)
set(Medium_SRCS
    src/ext2fs/do_check.cc
    src/ext2fs/do_read.cc
    src/ext2fs/do_readinode.cc
    src/ext2fs/do_writeinode.cc
    src/ext2fs/fuse_ext2.cc
    src/ext2fs/op_create.cc
    src/ext2fs/op_getattr.cc
    src/ext2fs/op_init.cc
    src/ext2fs/op_mkdir.cc
    src/ext2fs/op_open.cc
    src/ext2fs/op_readdir.cc
    src/ext2fs/op_release.cc
    src/ext2fs/op_rmdir.cc
    src/ext2fs/op_truncate.cc
    src/ext2fs/op_unlink.cc
    src/ext2fs/op_write.cc
    src/DataSplitterOutputStream.cc
    src/main.cc
    src/Path.cc
    src/RabinKarpHash.cc
)

include_directories(src ${CMAKE_CURRENT_BINARY_DIR}/include ${FLINT_BUILDROOT}/include)
link_directories(${FLINT_BUILDROOT}/lib)

add_executable(medium ${Medium_SRCS} )
target_link_libraries(medium pthread baseline fuse ext2fs)
target_compile_definitions(medium PRIVATE _FILE_OFFSET_BITS=64)