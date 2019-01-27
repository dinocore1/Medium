#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::op_release( const char* path, struct fuse_file_info* fi )
{
  errcode_t err;
  int ret;

  FileHandle* file = ( FileHandle* ) fi->fh;

  LOG_INFO( LOG_TAG, "path = %s", path );

  err = ext2fs_file_close( file->efile );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
  }

  if( file->open_flags & EXT2_FILE_WRITE ) {
    err = ext2fs_flush2( e2fs, EXT2_FLAG_FLUSH_NO_SYNC );
    if( err ) {
      ret = translate_error( e2fs, file->ino, err );
    }
  }

  delete file;
  fi->fh = 0;

  return 0;
}

}