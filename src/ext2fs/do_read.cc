#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::op_read( const char* path, char* buf, size_t len, off_t offset, struct fuse_file_info* fi )
{

  errcode_t err;
  unsigned int bytes_read;

  FileHandle* file = ( FileHandle* ) fi->fh;

  LOG_INFO( LOG_TAG, "path = %s", path );

  err = ext2fs_file_llseek( file->efile, offset, SEEK_SET, NULL );
  if( err ) {
    err = translate_error( e2fs, file->ino, err );
    return err;
  }

  err = ext2fs_file_read( file->efile, buf, len, &bytes_read );
  if( err ) {
    err = translate_error( e2fs, file->ino, err );
    return err;
  }

  return bytes_read;
}

}