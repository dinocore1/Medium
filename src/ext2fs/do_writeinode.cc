#include "ext2fs/fuse_ext2.h"

namespace medium {


int Ext2FS::do_writeinode( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode )
{
  int rt;
  errcode_t rc;
  if( inode->i_links_count < 1 ) {
    rt = do_killfilebyinode( e2fs, ino, inode );
    if( rt ) {
      LOG_DEBUG( LOG_TAG, "do_killfilebyinode(e2fs, ino, inode); failed" );
      return rt;
    }

  } else {
    rc = ext2fs_write_inode( e2fs, ino, inode );
    if( rc ) {
      LOG_DEBUG( LOG_TAG, "ext2fs_read_inode(e2fs, *ino, inode); failed" );
      return -EIO;
    }
  }
  return 0;
}

}