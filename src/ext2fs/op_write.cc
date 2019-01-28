#include "ext2fs/fuse_ext2_int.h"

namespace medium {



int Ext2FS::op_write( const char* path, const char* buf, size_t len, off_t offset, struct fuse_file_info* fi )
{
  errcode_t err;
  int ret;
  unsigned int bytes_written;

  FileHandle* file = ( FileHandle* ) fi->fh;

  LOG_INFO( LOG_TAG, "path = %s", path );

  if( !fs_can_allocate( e2fs, len / e2fs->blocksize ) ) {
    ret = -ENOSPC;
    return ret;
  }

  err = ext2fs_file_llseek( file->efile, offset, SEEK_SET, NULL );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
    return ret;
  }

  err = ext2fs_file_write( file->efile, buf, len, &bytes_written );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
    return ret;
  }

  err = ext2fs_file_flush( file->efile );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
    return ret;
  }

  return bytes_written;
}

}