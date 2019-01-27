#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::op_unlink( const char* path )
{
  ext2_ino_t ino;
  errcode_t err;
  int ret = 0;

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &ino );
  if( err ) {
    ret = translate_error( e2fs, 0, err );
    goto out;
  }

  ret = unlink_file_by_name( e2fs, path );
  if( ret ) {
    goto out;
  }

  ret = remove_inode( e2fs, ino );
  if( ret ) {
    goto out;
  }
out:
  return ret;
}

} // medium
