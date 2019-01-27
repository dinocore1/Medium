#include "ext2fs/fuse_ext2.h"

namespace medium {

int Ext2FS::op_unlink( const char* path )
{
  int rt;
  errcode_t rc;

  char* p_path;
  char* r_path;

  ext2_ino_t p_ino;
  ext2_ino_t r_ino;
  struct ext2_inode p_inode;
  struct ext2_inode r_inode;


  LOG_INFO( LOG_TAG, "path = %s", path );

  rt = do_check( path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check(%s); failed", path );
    return rt;
  }

  rt = do_check_split( path, &p_path, &r_path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check_split: failed" );
    return rt;
  }

  LOG_DEBUG( LOG_TAG, "parent: %s, child: %s", p_path, r_path );

  rt = do_readinode( e2fs, p_path, &p_ino, &p_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &p_ino, &p_inode); failed", path );
    free_split( p_path, r_path );
    return rt;
  }
  rt = do_readinode( e2fs, path, &r_ino, &r_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &r_ino, &r_inode); failed", path );
    free_split( p_path, r_path );
    return rt;
  }

  if( LINUX_S_ISDIR( r_inode.i_mode ) ) {
    LOG_DEBUG( LOG_TAG, "%s is a directory", path );
    free_split( p_path, r_path );
    return -EISDIR;
  }

  rc = ext2fs_unlink( e2fs, p_ino, r_path, r_ino, 0 );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_unlink(e2fs, %d, %s, %d, 0); failed", p_ino, r_path, r_ino );
    free_split( p_path, r_path );
    return -EIO;
  }

  p_inode.i_ctime = p_inode.i_mtime = e2fs->now ? e2fs->now : time( NULL );
  rt = do_writeinode( e2fs, p_ino, &p_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_writeinode(e2fs, p_ino, &p_inode); failed" );
    free_split( p_path, r_path );
    return -EIO;
  }

  do_remove_inode(e2fs, r_ino);

  
  free_split( p_path, r_path );

  return 0;
}

} // medium
