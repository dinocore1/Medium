#include "ext2fs/fuse_ext2.h"

namespace medium {

struct rd_struct {
  ext2_ino_t	parent;
  int		empty;
};

static int rmdir_proc( ext2_ino_t dir EXT2FS_ATTR( ( unused ) ),
                       int entry EXT2FS_ATTR( ( unused ) ),
                       struct ext2_dir_entry* dirent,
                       int offset EXT2FS_ATTR( ( unused ) ),
                       int blocksize EXT2FS_ATTR( ( unused ) ),
                       char* buf EXT2FS_ATTR( ( unused ) ), void* private_data )
{
  struct rd_struct* rds = ( struct rd_struct* ) private_data;

  if( dirent->inode == 0 ) {
    return 0;
  }
  if( ( ( dirent->name_len & 0xFF ) == 1 ) && ( dirent->name[0] == '.' ) ) {
    return 0;
  }
  if( ( ( dirent->name_len & 0xFF ) == 2 ) && ( dirent->name[0] == '.' ) &&
      ( dirent->name[1] == '.' ) ) {
    rds->parent = dirent->inode;
    return 0;
  }
  rds->empty = 0;
  return 0;
}


int Ext2FS::op_rmdir( const char* path )
{
  ext2_ino_t child;
  errcode_t err;
  struct ext2_inode_large inode;
  struct rd_struct rds;
  int ret = 0;

  LOG_INFO( LOG_TAG, "path=%s", path );

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &child );
  if( err ) {
    ret = translate_error( e2fs, 0, err );
    goto out;
  }

  rds.parent = 0;
  rds.empty = 1;

  err = ext2fs_dir_iterate2( e2fs, child, 0, 0, rmdir_proc, &rds );
  if( err ) {
    ret = translate_error( e2fs, child, err );
    goto out;
  }

  if( rds.empty == 0 ) {
    ret = -ENOTEMPTY;
    goto out;
  }

  ret = unlink_file_by_name( e2fs, path );
  if( ret ) {
    goto out;
  }
  /* Directories have to be "removed" twice. */
  ret = remove_inode( e2fs, child );
  if( ret ) {
    goto out;
  }
  ret = remove_inode( e2fs, child );
  if( ret ) {
    goto out;
  }

  if( rds.parent ) {
    LOG_DEBUG( LOG_TAG,  "decr dir=%d link count", rds.parent );
    err = ext2fs_read_inode_full( e2fs, rds.parent,
                                  ( struct ext2_inode* )&inode,
                                  sizeof( inode ) );
    if( err ) {
      ret = translate_error( e2fs, rds.parent, err );
      goto out;
    }
    if( inode.i_links_count > 1 ) {
      inode.i_links_count--;
    }
    ret = update_mtime( e2fs, rds.parent, &inode );
    if( ret ) {
      goto out;
    }
    err = ext2fs_write_inode_full( e2fs, rds.parent,
                                   ( struct ext2_inode* )&inode,
                                   sizeof( inode ) );
    if( err ) {
      ret = translate_error( e2fs, rds.parent, err );
      goto out;
    }
  }

out:
  return ret;
}

} // medium

