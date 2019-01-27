#include "ext2fs/fuse_ext2.h"

namespace medium {


const char* Ext2FS::LOG_TAG = "Ext2FS";

static void get_now( struct timespec* now )
{
#ifdef CLOCK_REALTIME
  if( !clock_gettime( CLOCK_REALTIME, now ) ) {
    return;
  }
#endif

  now->tv_sec = time( NULL );
  now->tv_nsec = 0;
}

int Ext2FS::open( const char* filepath, int flags )
{
  int err;
  e2fs = &filsys;
  err = ext2fs_open( filepath, flags, 0, 0, unix_io_manager, &e2fs );
  if( err != 0 ) {
    LOG_ERROR( LOG_TAG, "could not open ext2 image: %s", filepath );
    return err;
  }

  err = ext2fs_read_bitmaps( e2fs );
  if( err != 0 ) {
    LOG_ERROR( LOG_TAG, "error reading bitmaps: %d", err );
  }

  return err;
}

int Ext2FS::do_remove_inode( ext2_filsys e2fs, ext2_ino_t ino )
{
  struct ext2_inode_large inode;
  errcode_t err;
  int ret = 0;

  memset( &inode, 0, sizeof( inode ) );
  err = ext2fs_read_inode_full( e2fs, ino, ( struct ext2_inode* )&inode, sizeof( inode ) );


  if( err ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_read_inode(e2fs, *ino, inode); failed" );
    return -EIO;
  }

  switch( inode.i_links_count ) {
  case 0:
    LOG_WARN( LOG_TAG, "already done?" );
    return 0;

  case 1:
    inode.i_links_count--;
    inode.i_dtime = e2fs->now ? e2fs->now : time( 0 );
    break;

  default:
    inode.i_links_count--;
  }

  if( inode.i_links_count ) {
    goto write_out;
  }

  /* Nobody holds this file; free its blocks! */
  err = ext2fs_free_ext_attr( e2fs, ino, &inode );
  if( err ) {
    goto write_out;
  }

  if( ext2fs_inode_has_valid_blocks2( e2fs, ( struct ext2_inode* )&inode ) ) {
    err = ext2fs_punch( e2fs, ino, ( struct ext2_inode* )&inode, NULL,
                        0, ~0ULL );
    if( err ) {
      ret = translate_error( e2fs, ino, err );
      goto write_out;
    }
  }

  ext2fs_inode_alloc_stats2( e2fs, ino, -1,
                             LINUX_S_ISDIR( inode.i_mode ) );

write_out:
  err = ext2fs_write_inode_full( e2fs, ino, ( struct ext2_inode* )&inode,
                                 sizeof( inode ) );
  if( err ) {
    ret = translate_error( e2fs, ino, err );
    goto out;
  }
out:
  return ret;

}

static const char* error_message( long i )
{
  return "UNKNOWN_ERROR";
}

int Ext2FS::__translate_error( ext2_filsys fs, errcode_t err, ext2_ino_t ino,
                               const char* file, int line )
{
  struct timespec now;
  int ret = err;
  int is_err = 0;

  /* Translate ext2 error to unix error code */
  if( err < EXT2_ET_BASE ) {
    goto no_translation;
  }
  switch( err ) {
  case EXT2_ET_NO_MEMORY:
  case EXT2_ET_TDB_ERR_OOM:
    ret = -ENOMEM;
    break;
  case EXT2_ET_INVALID_ARGUMENT:
  case EXT2_ET_LLSEEK_FAILED:
    ret = -EINVAL;
    break;
  case EXT2_ET_NO_DIRECTORY:
    ret = -ENOTDIR;
    break;
  case EXT2_ET_FILE_NOT_FOUND:
    ret = -ENOENT;
    break;
  case EXT2_ET_DIR_NO_SPACE:
    is_err = 1;
  /* fallthrough */
  case EXT2_ET_TOOSMALL:
  case EXT2_ET_BLOCK_ALLOC_FAIL:
  case EXT2_ET_INODE_ALLOC_FAIL:
  case EXT2_ET_EA_NO_SPACE:
    ret = -ENOSPC;
    break;
  case EXT2_ET_SYMLINK_LOOP:
    ret = -EMLINK;
    break;
  case EXT2_ET_FILE_TOO_BIG:
    ret = -EFBIG;
    break;
  case EXT2_ET_TDB_ERR_EXISTS:
  case EXT2_ET_FILE_EXISTS:
    ret = -EEXIST;
    break;
  case EXT2_ET_MMP_FAILED:
  case EXT2_ET_MMP_FSCK_ON:
    ret = -EBUSY;
    break;
  case EXT2_ET_EA_KEY_NOT_FOUND:
#ifdef ENODATA
    ret = -ENODATA;
#else
    ret = -ENOENT;
#endif
    break;
  /* Sometimes fuse returns a garbage file handle pointer to us... */
  case EXT2_ET_MAGIC_EXT2_FILE:
    ret = -EFAULT;
    break;
  case EXT2_ET_UNIMPLEMENTED:
    ret = -EOPNOTSUPP;
    break;
  default:
    is_err = 1;
    ret = -EIO;
    break;
  }

no_translation:
  if( !is_err ) {
    return ret;
  }

  if( ino )
    LOG_ERROR( LOG_TAG, "FUSE2FS (%s): %s (inode #%d) at %s:%d.",
               fs->device_name ? fs->device_name : "???",
               error_message( err ), ino, file, line );
  else
    LOG_ERROR( LOG_TAG, "FUSE2FS (%s): %s at %s:%d.",
               fs->device_name ? fs->device_name : "???",
               error_message( err ), file, line );


  /* Make a note in the error log */
  get_now( &now );
  fs->super->s_last_error_time = now.tv_sec;
  fs->super->s_last_error_ino = ino;
  fs->super->s_last_error_line = line;
  fs->super->s_last_error_block = err; /* Yeah... */
  strncpy( ( char* )fs->super->s_last_error_func, file,
           sizeof( fs->super->s_last_error_func ) );
  if( fs->super->s_first_error_time == 0 ) {
    fs->super->s_first_error_time = now.tv_sec;
    fs->super->s_first_error_ino = ino;
    fs->super->s_first_error_line = line;
    fs->super->s_first_error_block = err;
    strncpy( ( char* )fs->super->s_first_error_func, file,
             sizeof( fs->super->s_first_error_func ) );
  }

  fs->super->s_error_count++;
  ext2fs_mark_super_dirty( fs );
  ext2fs_flush( fs );

  /*
  if (ff->panic_on_error)
  abort();
  */

  return ret;
}

}