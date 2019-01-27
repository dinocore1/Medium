
#include "ext2fs/fuse_ext2.h"

namespace medium {

int Ext2FS::op_mkdir( const char* path, mode_t mode )
{
  int rt;
  time_t tm;
  errcode_t rc;

  char* p_path;
  char* r_path;

  ext2_ino_t ino;
  struct ext2_inode inode;

  struct fuse_context* ctx;

  LOG_INFO( LOG_TAG, "path = %s, mode: 0%o, dir:0%o", path, mode, LINUX_S_IFDIR );

  rt = do_check_split( path, &p_path, &r_path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check(%s); failed", path );
    return rt;
  }

  rt = do_readinode( e2fs, p_path, &ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &ino, &inode); failed", p_path );
    free_split( p_path, r_path );
    return rt;
  }

  do {
    LOG_DEBUG( LOG_TAG, "calling ext2fs_mkdir(e2fs, %d, 0, %s);", ino, r_path );
    rc = ext2fs_mkdir( e2fs, ino, 0, r_path );
    if( rc == EXT2_ET_DIR_NO_SPACE ) {
      LOG_DEBUG( LOG_TAG, "calling ext2fs_expand_dir(e2fs, &d)", ino );
      if( ext2fs_expand_dir( e2fs, ino ) ) {
        LOG_DEBUG( LOG_TAG, "error while expanding directory %s (%d)", p_path, ino );
        free_split( p_path, r_path );
        return -ENOSPC;
      }
    }
  } while( rc == EXT2_ET_DIR_NO_SPACE );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_mkdir(e2fs, %d, 0, %s); failed (%d)", ino, r_path, rc );
    LOG_DEBUG( LOG_TAG, "e2fs: %p, e2fs->inode_map: %p", e2fs, e2fs->inode_map );
    free_split( p_path, r_path );
    return -EIO;
  }

  rt = do_readinode( e2fs, path, &ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &ino, &inode); failed", path );
    free_split( p_path, r_path );
    return -EIO;
  }
  tm = e2fs->now ? e2fs->now : time( NULL );
  inode.i_mode = LINUX_S_IFDIR | mode;
  inode.i_ctime = inode.i_atime = inode.i_mtime = tm;
  /*
  ctx = fuse_get_context();
  if (ctx) {
  	ext2_write_uid(&inode, ctx->uid);
  	ext2_write_gid(&inode, ctx->gid);
  }
  */
  rc = do_writeinode( e2fs, ino, &inode );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "do_writeinode(e2fs, ino, &inode); failed" );
    free_split( p_path, r_path );
    return -EIO;
  }

  /* update parent dir */
  rt = do_readinode( e2fs, p_path, &ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &ino, &inode); dailed", p_path );
    free_split( p_path, r_path );
    return -EIO;
  }
  inode.i_ctime = inode.i_mtime = tm;
  rc = do_writeinode( e2fs, ino, &inode );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "do_writeinode(e2fs, ino, &inode); failed" );
    free_split( p_path, r_path );
    return -EIO;
  }

  free_split( p_path, r_path );

  return 0;
}

}