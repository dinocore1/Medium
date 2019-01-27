#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::do_readinode( ext2_filsys e2fs, const char* path, ext2_ino_t* ino, struct ext2_inode* inode )
{
  errcode_t rc;
  rc = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, ino );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_namei(e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, %s, ino); failed", path );
    return -ENOENT;
  }
  rc = ext2fs_read_inode( e2fs, *ino, inode );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_read_inode(e2fs, *ino, inode); failed" );
    return -EIO;
  }
  return 0;
}

} // medium
