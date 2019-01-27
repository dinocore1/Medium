#include "ext2fs/fuse_ext2.h"

namespace medium {


int Ext2FS::do_release( ext2_file_t efile )
{
  errcode_t rc;


  LOG_DEBUG( LOG_TAG, "path = (%p)", efile );

  if( efile == NULL ) {
    return -ENOENT;
  }
  rc = ext2fs_file_close( efile );
  if( rc ) {
    return -EIO;
  }


  return 0;
}

int Ext2FS::op_release( const char* path, struct fuse_file_info* fi )
{
  int rt;
  ext2_file_t efile = ( ext2_file_t )( unsigned long ) fi->fh;

  LOG_INFO( LOG_TAG, "path = %s (%p)", path, efile );
  rt = do_release( efile );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_release() failed" );
    return rt;
  }


  return 0;
}

}