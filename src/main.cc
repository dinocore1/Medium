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
#include <baseline/ExecutorService.h>

#include <set>

#include "Path.h"
#include "ScanTask.h"

using namespace medium;

using namespace baseline;

using std::set;

sp<ExecutorService> mScanQueue;

class LiveExt2FS : public Ext2FS
{
public:
  //int op_open( const char* path, struct fuse_file_info* fi );
  int op_release( const char* path, struct fuse_file_info* fi );
  int op_write( const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi );

  set<ext2_ino_t> mOpenWritableFiles;

private:


};

/*
int LiveExt2FS::op_open(const char* path, struct fuse_file_info* fi)
{
  int retval = Ext2FS::op_open(path, fi);
  if(retval == 0 && fi->flags & O_WRONLY || fi->flags & O_RDWR) {
    FileHandle* file = (FileHandle*) fi->fh;
    mOpenWritableFiles.emplace(file->ino);
  }

  return retval;
}
*/

int LiveExt2FS::op_write( const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi )
{
  FileHandle* file = ( FileHandle* ) fi->fh;
  ext2_ino_t ino = file->ino;
  int retval = Ext2FS::op_write( path, buf, size, offset, fi );
  mOpenWritableFiles.emplace( ino );
  return retval;
}

int LiveExt2FS::op_release( const char* path, struct fuse_file_info* fi )
{
  FileHandle* file = ( FileHandle* ) fi->fh;
  ext2_ino_t ino = file->ino;
  int retval = Ext2FS::op_release( path, fi );
  if( retval == 0 && mOpenWritableFiles.erase( ino ) ) {
    LOG_INFO( "", "queue ino for scanning: %d", ino );
    mScanQueue->schedule( new ScanTask( *this, ino ), 1000 );
  }
}

LiveExt2FS mLiveFS;

static void* do_init( fuse_conn_info* info )
{
  mLiveFS.op_init();
  return NULL;
}

static void do_destroy( void* p )
{
  mLiveFS.op_destroy();
}

static int do_getattr( const char* path, struct stat* st )
{
  return mLiveFS.op_getattr( path, st );
}

static int do_readdir( const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi )
{
  return mLiveFS.op_readdir( path, buffer, filler, offset, fi );
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

static int do_read( const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi )
{
  return mLiveFS.op_read( path, buffer, size, offset, fi );
}

int do_write( const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi )
{
  return mLiveFS.op_write( path, buf, size, offset, fi );
}

int do_truncate( const char* path, off_t length )
{
  return mLiveFS.op_truncate( path, length );
}

int do_ftruncate( const char* path, off_t length, struct fuse_file_info* fi )
{
  return mLiveFS.op_ftruncate( path, length, fi );
}

int do_unlink( const char* path )
{
  return mLiveFS.op_unlink( path );
}

int do_rmdir( const char* path )
{
  return mLiveFS.op_rmdir( path );
}

static struct fuse_operations operations;


int main( int argc, char* argv[] )
{
  int err;

  err = mLiveFS.open( "file.fs", EXT2_FLAG_RW );
  if( err ) {
    return err;
  }

  /*
  err = mPartsFS.open("parts.fs", EXT2_FLAG_RW);
  if(err) {
    return err;
  }
  */

  mScanQueue = ExecutorService::createExecutorService( String8( "scanner" ) );

  operations.init = do_init;
  operations.destroy = do_destroy;
  operations.getattr = do_getattr;
  operations.readdir = do_readdir;
  operations.mkdir = do_mkdir;
  operations.open = do_open;
  operations.release = do_release;
  operations.create = do_create;
  operations.read = do_read;
  operations.write = do_write;
  operations.truncate = do_truncate;
  operations.ftruncate = do_ftruncate;
  operations.unlink = do_unlink;
  operations.rmdir = do_rmdir;

  return fuse_main( argc, argv, &operations, NULL );

  LOG_INFO( "", "fuse_main exit" );

  mScanQueue->shutdown();

  LOG_INFO( "", "mScanQueue shutdown" );

}