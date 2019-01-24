#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <db_cxx.h>

#include <baseline/Baseline.h>

#include "DatablockDB.h"
#include "InodeDB.h"

using namespace medium;

DatablockDB* gDatablockDB;
InodeDB* gInodeDB;

static int do_getattr( const char* path, struct stat* st )
{
  LOG_INFO( "", "do_getattr(%s)", path );
  int ret = gInodeDB->getAttr( path, st );


  return ret;
}

static int do_readdir( const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi )
{
  LOG_INFO( "", "do_readdir(%s)", path );
  if( strcmp( path, "/" ) != 0 ) {
    return -ENOENT;
  }

  filler( buffer, ".", NULL, 0 );
  filler( buffer, "..", NULL, 0 );

  return 0;
}

static int do_read( const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi )
{
  LOG_INFO( "", "do_read(%s)", path );
}

static struct fuse_operations operations;



int main( int argc, char* argv[] )
{
  int err;

  DbEnv* dbenv = new DbEnv( 0 );
  //err = dbenv->add_data_dir( "/home/paul/.medium" );
  err = dbenv->open( "/home/paul/.medium",
                     DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
                     DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_THREAD,
                     0664 );

  {
    Db* db = new Db( dbenv, 0 );
    err = db->open( NULL, "datablocks.db", NULL, DB_BTREE, DB_CREATE, 0664 );
    gDatablockDB = new DatablockDB( db );
  }

  {
    Db* db = new Db( dbenv, 0 );
    err = db->open( NULL, "inode.db", NULL, DB_HEAP, DB_CREATE, 0664 );
    gInodeDB = new InodeDB( db );
  }

  operations.getattr = do_getattr;
  operations.readdir = do_readdir;
  operations.read = do_read;

  return fuse_main( argc, argv, &operations, NULL );

}