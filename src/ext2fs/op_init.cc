#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::op_init()
{
  errcode_t err;
  LOG_INFO( LOG_TAG, "dev=%s", e2fs->device_name );

  if( e2fs->flags & EXT2_FLAG_RW ) {
    e2fs->super->s_mnt_count++;
    e2fs->super->s_mtime = time( NULL );
    e2fs->super->s_state &= ~EXT2_VALID_FS;
    ext2fs_mark_super_dirty( e2fs );
    err = ext2fs_flush2( e2fs, 0 );
    if( err ) {
      translate_error( e2fs, 0, err );
    }
  }

  return err;
}

int Ext2FS::op_destroy()
{
  errcode_t err;
  LOG_INFO( LOG_TAG, "dev=%s", e2fs->device_name );

  if( e2fs->flags & EXT2_FLAG_RW ) {
    e2fs->super->s_state |= EXT2_VALID_FS;
    if( e2fs->super->s_error_count ) {
      e2fs->super->s_state |= EXT2_ERROR_FS;
    }
    ext2fs_mark_super_dirty( e2fs );
    err = ext2fs_set_gdt_csum( e2fs );
    if( err ) {
      translate_error( e2fs, 0, err );
    }

    err = ext2fs_flush2( e2fs, 0 );
    if( err ) {
      translate_error( e2fs, 0, err );
    }
  }

  return err;
}

} // medium



