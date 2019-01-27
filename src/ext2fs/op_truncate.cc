#include "ext2fs/fuse_ext2.h"

namespace medium {

int Ext2FS::do_truncate( ext2_filsys e2fs, ext2_file_t efile, const char* path, off_t length )
{
  int rt;
  ext2_ino_t ino;
  struct ext2_inode inode;
  errcode_t rc;

  rc = ext2fs_file_set_size2( efile, length );
  if( rc ) {
    do_release( efile );
    LOG_DEBUG( LOG_TAG, "ext2fs_file_set_size(efile, %d); failed", length );
    if( rc == EXT2_ET_FILE_TOO_BIG ) {
      return -EFBIG;
    }
    return -EIO;
  }

  rt = do_readinode( e2fs, path, &ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &ino, &vnode); failed", path );
    do_release( efile );
    return rt;
  }
  inode.i_ctime = e2fs->now ? e2fs->now : time( NULL );
  rt = do_writeinode( e2fs, ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_writeinode(e2fs, ino, &inode); failed" );
    do_release( efile );
    return -EIO;
  }
}

int Ext2FS::op_truncate( const char* path, off_t length )
{
  int rt;
  ext2_file_t efile;


  LOG_INFO( LOG_TAG, "path = %s", path );

  rt = do_check( path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check(%s); failed", path );
    return rt;
  }
  efile = do_open( e2fs, path, O_WRONLY );
  if( efile == NULL ) {
    LOG_DEBUG( LOG_TAG, "do_open(%s); failed", path );
    return -ENOENT;
  }

  rt = do_truncate( e2fs, efile, path, length );

  rt = do_release( efile );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_release(efile); failed" );
    return rt;
  }

  return 0;
}

int Ext2FS::op_ftruncate( const char* path, off_t length, struct fuse_file_info* fi )
{
  int rt;
  ext2_file_t efile = EXT2FS_FILE( fi->fh );

  LOG_INFO( LOG_TAG, "path = %s", path );

  rt = do_truncate( e2fs, efile, path, length );

  return rt;
}

}