#include "ext2fs/fuse_ext2_int.h"

namespace medium {


const char* Ext2FS::LOG_TAG = "Ext2FS";

/*
 * Extended fields will fit into an inode if the filesystem was formatted
 * with large inodes (-I 256 or larger) and there are not currently any EAs
 * consuming all of the available space. For new inodes we always reserve
 * enough space for the kernel's known extended fields, but for inodes
 * created with an old kernel this might not have been the case. None of
 * the extended inode fields is critical for correct filesystem operation.
 * This macro checks if a certain field fits in the inode. Note that
 * inode-size = GOOD_OLD_INODE_SIZE + i_extra_isize
 */
#define EXT4_FITS_IN_INODE(ext4_inode, field)		\
  ((offsetof(typeof(*ext4_inode), field) +	\
    sizeof((ext4_inode)->field))			\
   <= ((size_t) EXT2_GOOD_OLD_INODE_SIZE +		\
       (ext4_inode)->i_extra_isize))		\

static inline __u32 ext4_encode_extra_time( const struct timespec* time )
{
  __u32 extra = sizeof( time->tv_sec ) > 4 ?
                ( ( time->tv_sec - ( __s32 )time->tv_sec ) >> 32 ) &
                EXT4_EPOCH_MASK : 0;
  return extra | ( time->tv_nsec << EXT4_EPOCH_BITS );
}

static inline void ext4_decode_extra_time( struct timespec* time, __u32 extra )
{
  if( sizeof( time->tv_sec ) > 4 && ( extra & EXT4_EPOCH_MASK ) ) {
    __u64 extra_bits = extra & EXT4_EPOCH_MASK;
    /*
     * Prior to kernel 3.14?, we had a broken decode function,
     * wherein we effectively did this:
     * if (extra_bits == 3)
     *     extra_bits = 0;
     */
    time->tv_sec += extra_bits << 32;
  }
  time->tv_nsec = ( ( extra ) & EXT4_NSEC_MASK ) >> EXT4_EPOCH_BITS;
}

#define EXT4_INODE_SET_XTIME(xtime, timespec, raw_inode)		       \
  do {									       \
    (raw_inode)->xtime = (timespec)->tv_sec;			       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime ## _extra))		       \
      (raw_inode)->xtime ## _extra =				       \
          ext4_encode_extra_time(timespec);	       \
  } while (0)

#define EXT4_EINODE_SET_XTIME(xtime, timespec, raw_inode)		       \
  do {									       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime))			       \
      (raw_inode)->xtime = (timespec)->tv_sec;		       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime ## _extra))		       \
      (raw_inode)->xtime ## _extra =				       \
          ext4_encode_extra_time(timespec);	       \
  } while (0)

#define EXT4_INODE_GET_XTIME(xtime, timespec, raw_inode)		       \
  do {									       \
    (timespec)->tv_sec = (signed)((raw_inode)->xtime);		       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime ## _extra))		       \
      ext4_decode_extra_time((timespec),			       \
                             (raw_inode)->xtime ## _extra);	       \
    else								       \
      (timespec)->tv_nsec = 0;				       \
  } while (0)

#define EXT4_EINODE_GET_XTIME(xtime, timespec, raw_inode)		       \
  do {									       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime))			       \
      (timespec)->tv_sec =					       \
                                           (signed)((raw_inode)->xtime);			       \
    if (EXT4_FITS_IN_INODE(raw_inode, xtime ## _extra))		       \
      ext4_decode_extra_time((timespec),			       \
                             raw_inode->xtime ## _extra);	       \
    else								       \
      (timespec)->tv_nsec = 0;				       \
  } while (0)

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

static void increment_version( struct ext2_inode_large* inode )
{
  __u64 ver;

  ver = inode->osd1.linux1.l_i_version;
  if( EXT4_FITS_IN_INODE( inode, i_version_hi ) ) {
    ver |= ( __u64 )inode->i_version_hi << 32;
  }
  ver++;
  inode->osd1.linux1.l_i_version = ver;
  if( EXT4_FITS_IN_INODE( inode, i_version_hi ) ) {
    inode->i_version_hi = ver >> 32;
  }
}

void Ext2FS::init_times( struct ext2_inode_large* inode )
{
  struct timespec now;

  get_now( &now );
  EXT4_INODE_SET_XTIME( i_atime, &now, inode );
  EXT4_INODE_SET_XTIME( i_ctime, &now, inode );
  EXT4_INODE_SET_XTIME( i_mtime, &now, inode );
  EXT4_EINODE_SET_XTIME( i_crtime, &now, inode );
  increment_version( inode );
}

int Ext2FS::update_ctime( ext2_filsys fs, ext2_ino_t ino,
                          struct ext2_inode_large* pinode )
{
  errcode_t err;
  struct timespec now;
  struct ext2_inode_large inode;

  get_now( &now );

  /* If user already has a inode buffer, just update that */
  if( pinode ) {
    increment_version( pinode );
    EXT4_INODE_SET_XTIME( i_ctime, &now, pinode );
    return 0;
  }

  /* Otherwise we have to read-modify-write the inode */
  memset( &inode, 0, sizeof( inode ) );
  err = ext2fs_read_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  increment_version( &inode );
  EXT4_INODE_SET_XTIME( i_ctime, &now, &inode );

  err = ext2fs_write_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                 sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  return 0;
}

int Ext2FS::update_atime( ext2_filsys fs, ext2_ino_t ino )
{
  errcode_t err;
  struct ext2_inode_large inode, *pinode;
  struct timespec atime, mtime, now;

  if( !( fs->flags & EXT2_FLAG_RW ) ) {
    return 0;
  }
  memset( &inode, 0, sizeof( inode ) );
  err = ext2fs_read_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  pinode = &inode;
  EXT4_INODE_GET_XTIME( i_atime, &atime, pinode );
  EXT4_INODE_GET_XTIME( i_mtime, &mtime, pinode );
  get_now( &now );
  /*
   * If atime is newer than mtime and atime hasn't been updated in thirty
   * seconds, skip the atime update.  Same idea as Linux "relatime".
   */
  if( atime.tv_sec >= mtime.tv_sec && atime.tv_sec >= now.tv_sec - 30 ) {
    return 0;
  }
  EXT4_INODE_SET_XTIME( i_atime, &now, &inode );

  err = ext2fs_write_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                 sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  return 0;
}

int Ext2FS::update_mtime( ext2_filsys fs, ext2_ino_t ino, struct ext2_inode_large* pinode )
{
  errcode_t err;
  struct ext2_inode_large inode;
  struct timespec now;

  if( pinode ) {
    get_now( &now );
    EXT4_INODE_SET_XTIME( i_mtime, &now, pinode );
    EXT4_INODE_SET_XTIME( i_ctime, &now, pinode );
    increment_version( pinode );
    return 0;
  }

  memset( &inode, 0, sizeof( inode ) );
  err = ext2fs_read_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  get_now( &now );
  EXT4_INODE_SET_XTIME( i_mtime, &now, &inode );
  EXT4_INODE_SET_XTIME( i_ctime, &now, &inode );
  increment_version( &inode );

  err = ext2fs_write_inode_full( fs, ino, ( struct ext2_inode* )&inode,
                                 sizeof( inode ) );
  if( err ) {
    return translate_error( fs, ino, err );
  }

  return 0;
}

int Ext2FS::fs_can_allocate( ext2_filsys fs, blk64_t num )
{
  blk64_t reserved;

  /*
  dbg_printf( "%s: Asking for %llu; alloc_all=%d total=%llu free=%llu "
              "rsvd=%llu\n", __func__, num, ff->alloc_all_blocks,
              ext2fs_blocks_count( fs->super ),
              ext2fs_free_blocks_count( fs->super ),
              ext2fs_r_blocks_count( fs->super ) );
  */

  if( num > ext2fs_blocks_count( fs->super ) ) {
    return 0;
  }

  /*
   * Different meaning for r_blocks -- libext2fs has bugs where the FS
   * can get corrupted if it totally runs out of blocks.  Avoid this
   * by refusing to allocate any of the reserve blocks to anybody.
   */
  reserved = ext2fs_r_blocks_count( fs->super );
  if( reserved == 0 ) {
    reserved = ext2fs_blocks_count( fs->super ) / 10;
  }
  return ext2fs_free_blocks_count( fs->super ) > reserved + num;
}

int Ext2FS::check_inum_access( ext2_filsys fs, ext2_ino_t ino, mode_t mask )
{
  struct ext2_inode inode;
  mode_t perms;
  errcode_t err;

  err = ext2fs_read_inode( fs, ino, &inode );
  if( err ) {
    return translate_error( fs, ino, err );
  }
  perms = inode.i_mode & 0777;

  LOG_INFO( LOG_TAG, "access ino=%d mask=e%s%s%s perms=0%o fuid=%d fgid=%d "
            "uid=%d gid=%d", ino,
            ( mask & R_OK ? "r" : "" ), ( mask & W_OK ? "w" : "" ),
            ( mask & X_OK ? "x" : "" ), perms, inode.i_uid, inode.i_gid,
            getuid(), getgid() );

  /* existence check */
  if( mask == 0 ) {
    return 0;
  }

  /* is immutable? */
  if( ( mask & W_OK ) &&
      ( inode.i_flags & EXT2_IMMUTABLE_FL ) ) {
    return -EACCES;
  }

  /* we assume to be the owner of all files */
  if( ( mask & ( perms >> 6 ) ) == mask ) {
    return 0;
  }
  return -EACCES;


}

int Ext2FS::unlink_file_by_name( ext2_filsys fs, const char* path )
{
  errcode_t err;
  ext2_ino_t dir;
  char* filename = strdup( path );
  char* base_name;
  int ret;

  base_name = strrchr( filename, '/' );
  if( base_name ) {
    *base_name++ = '\0';
    err = ext2fs_namei( fs, EXT2_ROOT_INO, EXT2_ROOT_INO, filename,
                        &dir );
    if( err ) {
      free( filename );
      return translate_error( fs, 0, err );
    }
  } else {
    dir = EXT2_ROOT_INO;
    base_name = filename;
  }

  ret = check_inum_access( fs, dir, W_OK );
  if( ret ) {
    free( filename );
    return ret;
  }

  LOG_INFO( LOG_TAG, "unlinking name=%s from dir=%d", base_name, dir );
  err = ext2fs_unlink( fs, dir, base_name, 0, 0 );
  free( filename );
  if( err ) {
    return translate_error( fs, dir, err );
  }

  return update_mtime( fs, dir, NULL );
}


int Ext2FS::remove_inode( ext2_filsys e2fs, ext2_ino_t ino )
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