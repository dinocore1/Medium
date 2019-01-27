#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <ext2fs/ext2fs.h>

#include "ext2fs/fuse_ext2.h"

#include <baseline/Baseline.h>

#include "Path.h"

using namespace medium;

Ext2FS mLiveFS;


static int do_getattr( const char* path, struct stat* st )
{
  return mLiveFS.op_getattr( path, st );
}

static int do_readdir( const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi )
{
  return mLiveFS.op_readdir( path, buffer, filler, offset, fi );
}

static int do_read( const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi )
{
  LOG_INFO( "", "path = %s, size = %d", path, size );
  return -1;
}

static int do_mkdir( const char* path, mode_t mode )
{
  return mLiveFS.op_mkdir( path, mode );
}

static int do_open( const char* path, struct fuse_file_info* fi )
{
  return mLiveFS.op_open( path, fi );
}

static int do_release( const char* path, struct fuse_file_info* fi )
{
  return mLiveFS.op_release( path, fi );
}

static int do_create( const char* path, mode_t mode, struct fuse_file_info* fi )
{
  return mLiveFS.op_create( path, mode, fi );
}

static struct fuse_operations operations;


int main( int argc, char* argv[] )
{
  int err;

  err = mLiveFS.open( "file.fs", EXT2_FLAG_RW );
  if( err != 0 ) {
    return -1;
  }



  operations.getattr = do_getattr;
  operations.readdir = do_readdir;
  operations.read = do_read;
  operations.mkdir = do_mkdir;
  operations.open = do_open;
  operations.release = do_release;
  operations.create = do_create;

  return fuse_main( argc, argv, &operations, NULL );

}