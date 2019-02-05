#include "ext2fs/fuse_ext2_int.h"

#include <baseline/UniquePointer.h>

using namespace baseline;

namespace medium {

#ifdef __linux__
static void detect_linux_executable_open( int kernel_flags, int* access_check,
    int* e2fs_open_flags )
{
  /*
   * On Linux, execve will bleed __FMODE_EXEC into the file mode flags,
   * and FUSE is more than happy to let that slip through.
   */
  if( kernel_flags & 0x20 ) {
    *access_check = X_OK;
    *e2fs_open_flags &= ~EXT2_FILE_WRITE;
  }
}
#else
static void detect_linux_executable_open( int kernel_flags, int* access_check,
    int* e2fs_open_flags )
{
  /* empty */
}
#endif /* __linux__ */

int Ext2FS::op_open( const char* path, struct fuse_file_info* fi )
{
  errcode_t err;
  int check = 0, ret = 0;

  LOG_INFO( LOG_TAG, "path = %s", path );

  up<FileHandle> file( new FileHandle() );
  file->open_flags = 0;
  switch( fi->flags & O_ACCMODE ) {
  case O_RDONLY:
    check = R_OK;
    break;
  case O_WRONLY:
    check = W_OK;
    file->open_flags |= EXT2_FILE_WRITE;
    break;
  case O_RDWR:
    check = R_OK | W_OK;
    file->open_flags |= EXT2_FILE_WRITE;
    break;
  }

  detect_linux_executable_open( fi->flags, &check, &file->open_flags );

  if( fi->flags & O_CREAT ) {
    file->open_flags |= EXT2_FILE_CREATE;
  }

  err = ext2fs_namei( e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &file->ino );
  if( err || file->ino == 0 ) {
    ret = translate_error( e2fs, 0, err );
    return ret;
  }

  ret = check_inum_access( e2fs, file->ino, check );
  if( ret ) {
    return ret;
  }

  ret = ext2fs_file_open( e2fs, file->ino, file->open_flags, &file->efile );
  if( ret ) {
    LOG_ERROR( LOG_TAG, "error ext2fs_file_open: 0x%x", ret );
    return ret;
  }

  FileHandle* handle = file.release();
  fi->fh = ( uint64_t ) handle;

  if(handle->open_flags & EXT2_FILE_WRITE) {
    mFiles[handle->ino] = handle;
  }

  return 0;
}


}