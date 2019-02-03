#include "ext2fs/fuse_ext2_int.h"

namespace medium {



int Ext2FS::op_truncate( const char* path, off_t len )
{
  errcode_t err;
  ext2_ino_t ino;
  ext2_file_t efile;
  int ret;

  LOG_INFO(LOG_TAG, "path=%s len=%d", path, len);

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &ino );
  if( err || ino == 0 ) {
    ret = translate_error( e2fs, 0, err );
    return ret;
  }

  ret = check_inum_access( e2fs, ino, W_OK );
  if( ret ) {
    return ret;
  }

  err = ext2fs_file_open( e2fs, ino, EXT2_FILE_WRITE, &efile );
  if( err ) {
    ret = translate_error( e2fs, ino, err );
    return ret;
  }

  err = ext2fs_file_set_size2( efile, len );
  if( err ) {
    ret = translate_error( e2fs, ino, err );
    return ret;
  }


  err = ext2fs_file_close( efile );
  if( ret ) {
    return ret;
  }
  if( err ) {
    ret = translate_error( e2fs, ino, err );
    return ret;
  }

  ret = update_mtime( e2fs, ino, NULL );
  if( ret ) {
    return ret;
  }

  return 0;
}

int Ext2FS::op_ftruncate( const char* path, off_t len, struct fuse_file_info* fi )
{
  errcode_t err;
  int ret;
  FileHandle* file = ( FileHandle* ) fi->fh;

  LOG_INFO( LOG_TAG, "path = %s", path );

  err = ext2fs_file_set_size2( file->efile, len );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
    return ret;
  }

  ret = update_mtime( e2fs, file->ino, NULL );
  if( ret ) {
    return ret;
  }

  return 0;
}

}