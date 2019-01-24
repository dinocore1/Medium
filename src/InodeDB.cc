

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <db_cxx.h>

#include <baseline/Baseline.h>
#include <baseline/Mutex.h>

#include "InodeDB.h"

using baseline::Mutex;

namespace medium {

uint64_t InodeDB::INode::getFileSize()
{
  return 0xFFFFFFFFFF & fileSizeFlags;
}

uint8_t InodeDB::INode::getFlags()
{
  return fileSizeFlags >> 56;
}


inline static
void createRootRID( DB_HEAP_RID& id )
{
  id.pgno = 2;
  id.indx = 0;
}

InodeDB::InodeDB( Db* db )
  : mDatabase( db )
{
}

int InodeDB::init()
{
  int err;
  //try to get the root inode

  DB_HEAP_RID rid;
  createRootRID( rid );
  Dbt key( &rid, DB_HEAP_RID_SZ );
  Dbt data;
  data.set_flags( DB_DBT_MALLOC );

  err = mDatabase->get( NULL, &key, &data, 0 );
  if( err == 0 ) {
    return err;
  }

  INode rootNode;
  memset( rootNode.name, 0, sizeof( rootNode.name ) );
  rootNode.fileSizeFlags = 0;
  rootNode.setFlags( S_IFDIR | 7 );

  err = mDatabase->put( NULL, &key, &data, DB_APPEND );
  if( err != 0 || ( rid.pgno != 2 && rid.indx != 0 ) {
  return -EINVAL;
}

return retval;
}

int InodeDB::getAttr( const char* path, struct stat* st )
{
  int ret;
  Mutex::Autolock lock( mLock );



  if( strcmp( path, "/" ) == 0 ) {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 2;
    ret = 0;
  } else {
    ret = -ENOENT;
  }

  return ret;
}

} // namespace

