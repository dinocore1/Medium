#include "ext2fs/fuse_ext2_int.h"

namespace medium {

struct update_dotdot {
  ext2_ino_t new_dotdot;
};

static int update_dotdot_helper( ext2_ino_t dir EXT2FS_ATTR( ( unused ) ),
                                 int entry EXT2FS_ATTR( ( unused ) ),
                                 struct ext2_dir_entry* dirent,
                                 int offset EXT2FS_ATTR( ( unused ) ),
                                 int blocksize EXT2FS_ATTR( ( unused ) ),
                                 char* buf EXT2FS_ATTR( ( unused ) ),
                                 void* priv_data )
{
  struct update_dotdot* ud = ( struct update_dotdot* ) priv_data;

  if( ext2fs_dirent_name_len( dirent ) == 2 &&
      dirent->name[0] == '.' && dirent->name[1] == '.' ) {
    dirent->inode = ud->new_dotdot;
    return DIRENT_CHANGED | DIRENT_ABORT;
  }

  return 0;
}

int Ext2FS::op_rename( const char* from, const char* to )
{
  errcode_t err;
  ext2_ino_t from_ino, to_ino, to_dir_ino, from_dir_ino;
  char* temp_to = NULL, *temp_from = NULL;
  char* cp, a;
  struct ext2_inode inode;
  struct update_dotdot ud;
  int ret = 0;


  LOG_INFO( LOG_TAG, "renaming %s to %s", from, to );

  if( !fs_can_allocate( e2fs, 5 ) ) {
    ret = -ENOSPC;
    goto out;
  }

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, from, &from_ino );
  if( err || from_ino == 0 ) {
    ret = translate_error( e2fs, 0, err );
    goto out;
  }

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, to, &to_ino );
  if( err && err != EXT2_ET_FILE_NOT_FOUND ) {
    ret = translate_error( e2fs, 0, err );
    goto out;
  }

  if( err == EXT2_ET_FILE_NOT_FOUND ) {
    to_ino = 0;
  }

  /* Already the same file? */
  if( to_ino != 0 && to_ino == from_ino ) {
    ret = 0;
    goto out;
  }

  temp_to = strdup( to );
  if( !temp_to ) {
    ret = -ENOMEM;
    goto out;
  }

  temp_from = strdup( from );
  if( !temp_from ) {
    ret = -ENOMEM;
    goto out2;
  }

  /* Find parent dir of the source and check write access */
  cp = strrchr( temp_from, '/' );
  if( !cp ) {
    ret = -EINVAL;
    goto out2;
  }

  a = *( cp + 1 );
  *( cp + 1 ) = 0;
  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, temp_from,
                      &from_dir_ino );
  *( cp + 1 ) = a;
  if( err ) {
    ret = translate_error( e2fs, 0, err );
    goto out2;
  }
  if( from_dir_ino == 0 ) {
    ret = -ENOENT;
    goto out2;
  }

  ret = check_inum_access( e2fs, from_dir_ino, W_OK );
  if( ret ) {
    goto out2;
  }

  /* Find parent dir of the destination and check write access */
  cp = strrchr( temp_to, '/' );
  if( !cp ) {
    ret = -EINVAL;
    goto out2;
  }

  a = *( cp + 1 );
  *( cp + 1 ) = 0;
  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, temp_to,
                      &to_dir_ino );
  *( cp + 1 ) = a;
  if( err ) {
    ret = translate_error( e2fs, 0, err );
    goto out2;
  }
  if( to_dir_ino == 0 ) {
    ret = -ENOENT;
    goto out2;
  }

  ret = check_inum_access( e2fs, to_dir_ino, W_OK );
  if( ret ) {
    goto out2;
  }

  /* If the target exists, unlink it first */
  if( to_ino != 0 ) {
    err = ext2fs_read_inode( e2fs, to_ino, &inode );
    if( err ) {
      ret = translate_error( e2fs, to_ino, err );
      goto out2;
    }

    LOG_DEBUG( LOG_TAG, "unlinking %s ino=%d",
               LINUX_S_ISDIR( inode.i_mode ) ? "dir" : "file",
               to_ino );
    if( LINUX_S_ISDIR( inode.i_mode ) ) {
      ret = op_rmdir( to );
    } else {
      ret = op_unlink( to );
    }
    if( ret ) {
      goto out2;
    }
  }

  /* Get ready to do the move */
  err = ext2fs_read_inode( e2fs, from_ino, &inode );
  if( err ) {
    ret = translate_error( e2fs, from_ino, err );
    goto out2;
  }

  /* Link in the new file */
  LOG_DEBUG( LOG_TAG, "linking ino=%d/path=%s to dir=%d\n",
             from_ino, cp + 1, to_dir_ino );
  err = ext2fs_link( e2fs, to_dir_ino, cp + 1, from_ino,
                     ext2_file_type( inode.i_mode ) );
  if( err == EXT2_ET_DIR_NO_SPACE ) {
    err = ext2fs_expand_dir( e2fs, to_dir_ino );
    if( err ) {
      ret = translate_error( e2fs, to_dir_ino, err );
      goto out2;
    }

    err = ext2fs_link( e2fs, to_dir_ino, cp + 1, from_ino,
                       ext2_file_type( inode.i_mode ) );
  }
  if( err ) {
    ret = translate_error( e2fs, to_dir_ino, err );
    goto out2;
  }

  /* Update '..' pointer if dir */
  err = ext2fs_read_inode( e2fs, from_ino, &inode );
  if( err ) {
    ret = translate_error( e2fs, from_ino, err );
    goto out2;
  }

  if( LINUX_S_ISDIR( inode.i_mode ) ) {
    ud.new_dotdot = to_dir_ino;
    LOG_DEBUG( LOG_TAG, "updating .. entry for dir=%d",
               to_dir_ino );
    err = ext2fs_dir_iterate2( e2fs, from_ino, 0, NULL,
                               update_dotdot_helper, &ud );
    if( err ) {
      ret = translate_error( e2fs, from_ino, err );
      goto out2;
    }

    /* Decrease from_dir_ino's links_count */
    LOG_DEBUG( LOG_TAG, "moving linkcount from dir=%d to dir=%d", from_dir_ino, to_dir_ino );
    err = ext2fs_read_inode( e2fs, from_dir_ino, &inode );
    if( err ) {
      ret = translate_error( e2fs, from_dir_ino, err );
      goto out2;
    }
    inode.i_links_count--;
    err = ext2fs_write_inode( e2fs, from_dir_ino, &inode );
    if( err ) {
      ret = translate_error( e2fs, from_dir_ino, err );
      goto out2;
    }

    /* Increase to_dir_ino's links_count */
    err = ext2fs_read_inode( e2fs, to_dir_ino, &inode );
    if( err ) {
      ret = translate_error( e2fs, to_dir_ino, err );
      goto out2;
    }
    inode.i_links_count++;
    err = ext2fs_write_inode( e2fs, to_dir_ino, &inode );
    if( err ) {
      ret = translate_error( e2fs, to_dir_ino, err );
      goto out2;
    }
  }

  /* Update timestamps */
  ret = update_ctime( e2fs, from_ino, NULL );
  if( ret ) {
    goto out2;
  }

  ret = update_mtime( e2fs, to_dir_ino, NULL );
  if( ret ) {
    goto out2;
  }

  /* Remove the old file */
  ret = unlink_file_by_name( e2fs, from );
  if( ret ) {
    goto out2;
  }

  /* Flush the whole mess out */
  err = ext2fs_flush2( e2fs, 0 );
  if( err ) {
    ret = translate_error( e2fs, 0, err );
  }

out2:
  free( temp_from );
  free( temp_to );
out:

  return ret;
}

} // medium
