#include "ext2fs/fuse_ext2.h"

namespace medium {

struct rmdir_st {
  ext2_ino_t parent;
  int empty;
};

static int rmdir_proc( ext2_ino_t dir EXT2FS_ATTR( ( unused ) ),
                       int entry EXT2FS_ATTR( ( unused ) ),
                       struct ext2_dir_entry* dirent,
                       int offset EXT2FS_ATTR( ( unused ) ),
                       int blocksize EXT2FS_ATTR( ( unused ) ),
                       char* buf EXT2FS_ATTR( ( unused ) ), void* private_data )
{
  int* p_empty = ( int* ) private_data;

  if( dirent->inode == 0 ||
      ( ( ( dirent->name_len & 0xFF ) == 1 ) && ( dirent->name[0] == '.' ) ) ||
      ( ( ( dirent->name_len & 0xFF ) == 2 ) && ( dirent->name[0] == '.' ) &&
        ( dirent->name[1] == '.' ) ) ) {

    return 0;
  }
  *p_empty = 0;

  return 0;
}

int Ext2FS::do_check_empty_dir( ext2_filsys e2fs, ext2_ino_t ino )
{
  errcode_t rc;
  int empty = 1;

  rc = ext2fs_dir_iterate2( e2fs, ino, 0, 0, rmdir_proc, &empty );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "while iterating over directory" );
    return -EIO;
  }

  if( empty == 0 ) {
    LOG_DEBUG( LOG_TAG, "directory not empty" );
    return -ENOTEMPTY;
  }

  return 0;
}

int Ext2FS::op_rmdir( const char* path )
{
  int rt;
  errcode_t rc;

  char* p_path;
  char* r_path;

  ext2_ino_t p_ino;
  struct ext2_inode p_inode;
  ext2_ino_t r_ino;
  struct ext2_inode r_inode;


  LOG_INFO( LOG_TAG, "path = %s", path );

  rt = do_check_split( path, &p_path, &r_path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check_split: failed" );
    return rt;
  }

  LOG_DEBUG( LOG_TAG, "parent: %s, child: %s", p_path, r_path );

  rt = do_readinode( e2fs, p_path, &p_ino, &p_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &p_ino, &p_inode); failed", p_path );
    free_split( p_path, r_path );
    return rt;
  }
  rt = do_readinode( e2fs, path, &r_ino, &r_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &r_ino, &r_inode); failed", path );
    free_split( p_path, r_path );
    return rt;

  }
  if( !LINUX_S_ISDIR( r_inode.i_mode ) ) {
    LOG_DEBUG( LOG_TAG, "%s is not a directory", path );
    free_split( p_path, r_path );
    return -ENOTDIR;
  }
  if( r_ino == EXT2_ROOT_INO ) {
    LOG_DEBUG( LOG_TAG, "root dir cannot be removed", path );
    free_split( p_path, r_path );
    return -EIO;
  }

  rt = do_check_empty_dir( e2fs, r_ino );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_check_empty_dir filed" );
    free_split( p_path, r_path );
    return rt;
  }

  rc = ext2fs_unlink( e2fs, p_ino, r_path, r_ino, 0 );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "while unlinking ino %d", ( int ) r_ino );
    free_split( p_path, r_path );
    return -EIO;
  }

  rt = do_remove_inode(e2fs, r_ino);
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_killfilebyinode(r_ino, &r_inode); failed" );
    free_split( p_path, r_path );
    return rt;
  }

  rt = do_readinode( e2fs, p_path, &p_ino, &p_inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(p_path, &p_ino, &p_inode); failed" );
    free_split( p_path, r_path );
    return rt;
  }
  if( p_inode.i_links_count > 1 ) {
    p_inode.i_links_count--;
  }
  p_inode.i_mtime = e2fs->now ? e2fs->now : time( NULL );
  p_inode.i_ctime = e2fs->now ? e2fs->now : time( NULL );
  rc = do_writeinode( e2fs, p_ino, &p_inode );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "do_writeinode(e2fs, ino, inode); failed" );
    free_split( p_path, r_path );
    return -EIO;
  }

  free_split( p_path, r_path );

  return 0;
}

} // medium

