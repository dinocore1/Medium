#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::ext2_file_type( unsigned int mode )
{
  if( LINUX_S_ISREG( mode ) ) {
    return EXT2_FT_REG_FILE;
  }

  if( LINUX_S_ISDIR( mode ) ) {
    return EXT2_FT_DIR;
  }

  if( LINUX_S_ISCHR( mode ) ) {
    return EXT2_FT_CHRDEV;
  }

  if( LINUX_S_ISBLK( mode ) ) {
    return EXT2_FT_BLKDEV;
  }

  if( LINUX_S_ISLNK( mode ) ) {
    return EXT2_FT_SYMLINK;
  }

  if( LINUX_S_ISFIFO( mode ) ) {
    return EXT2_FT_FIFO;
  }

  if( LINUX_S_ISSOCK( mode ) ) {
    return EXT2_FT_SOCK;
  }

  return 0;
}

int Ext2FS::op_create( const char* path, mode_t mode, struct fuse_file_info* fi )
{
  ext2_ino_t parent, child;
  char* temp_path;
  errcode_t err;
  char* node_name, a;
  int filetype;
  struct ext2_inode_large inode;
  int ret = 0;

  LOG_INFO( LOG_TAG, "path = %s, mode: 0%o", path, mode );


  temp_path = strdup( path );
  if( !temp_path ) {
    ret = -ENOMEM;
    goto out;
  }
  node_name = strrchr( temp_path, '/' );
  if( !node_name ) {
    ret = -ENOMEM;
    goto out;
  }
  node_name++;
  a = *node_name;
  *node_name = 0;

  if( !fs_can_allocate( e2fs, 1 ) ) {
    ret = -ENOSPC;
    goto out2;
  }

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, temp_path, &parent );
  if( err ) {
    ret = translate_error( e2fs, 0, err );
    goto out2;
  }

  ret = check_inum_access( e2fs, parent, W_OK );
  if( ret ) {
    goto out2;
  }

  *node_name = a;

  filetype = ext2_file_type( mode );

  err = ext2fs_new_inode( e2fs, parent, mode, 0, &child );
  if( err ) {
    ret = translate_error( e2fs, parent, err );
    goto out2;
  }

  LOG_DEBUG( LOG_TAG, "creating ino=%d/name=%s in dir=%d", child, node_name, parent );
  err = ext2fs_link( e2fs, parent, node_name, child, filetype );
  if( err == EXT2_ET_DIR_NO_SPACE ) {
    err = ext2fs_expand_dir( e2fs, parent );
    if( err ) {
      ret = translate_error( e2fs, parent, err );
      goto out2;
    }

    err = ext2fs_link( e2fs, parent, node_name, child, filetype );
  }
  if( err ) {
    ret = translate_error( e2fs, parent, err );
    goto out2;
  }

  ret = update_mtime( e2fs, parent, NULL );
  if( ret ) {
    goto out2;
  }

  memset( &inode, 0, sizeof( inode ) );
  inode.i_mode = mode;
  inode.i_links_count = 1;
  inode.i_extra_isize = sizeof( struct ext2_inode_large ) - EXT2_GOOD_OLD_INODE_SIZE;
  inode.i_uid = getuid();
  inode.i_gid = getgid();
  if( ext2fs_has_feature_extents( e2fs->super ) ) {
    ext2_extent_handle_t handle;

    inode.i_flags &= ~EXT4_EXTENTS_FL;
    ret = ext2fs_extent_open2( e2fs, child,
                               ( struct ext2_inode* )&inode, &handle );
    if( ret ) {
      return ret;
    }
    ext2fs_extent_free( handle );
  }

  err = ext2fs_write_new_inode( e2fs, child, ( struct ext2_inode* )&inode );
  if( err ) {
    ret = translate_error( e2fs, child, err );
    goto out2;
  }

  //inode.i_generation = ff->next_generation++;
  init_times( &inode );
  err = ext2fs_write_inode_full( e2fs, child, ( struct ext2_inode* )&inode,
                                 sizeof( inode ) );
  if( err ) {
    ret = translate_error( e2fs, child, err );
    goto out2;
  }

  ext2fs_inode_alloc_stats2( e2fs, child, 1, 0 );

  ret = op_open( path, fi );
  if( ret ) {
    goto out2;
  }
out2:
  //pthread_mutex_unlock(&ff->bfl);
out:
  free( temp_path );
  return ret;


}

}