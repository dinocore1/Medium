#include "ext2fs/fuse_ext2_int.h"
#include <baseline/UniquePointer.h>

using namespace baseline;

namespace medium {

int Ext2FS::op_release( const char* path, struct fuse_file_info* fi )
{
  errcode_t err;
  int ret;

  up<FileHandle> file(( FileHandle* ) fi->fh);

  LOG_INFO( LOG_TAG, "path = %s", path );

  if (file->open_flags & EXT2_FILE_WRITE) {
    ext2_ino_t ino = ext2fs_file_get_inode_num(file->efile);
    struct ext2_inode* inode = ext2fs_file_get_inode(file->efile);

    mFiles.erase(ino);

    err = ext2fs_write_inode(e2fs, ino, inode);
    if(err) {
      ret = translate_error(e2fs, file->ino, err);
      return ret;
    }

  }

  err = ext2fs_file_close( file->efile );
  if( err ) {
    ret = translate_error( e2fs, file->ino, err );
  }

  if( file->open_flags & EXT2_FILE_WRITE ) {

    /*
    ret = update_mtime(e2fs, file->ino, NULL);
	  if (ret) {
		  return ret;
    }
    */

    err = ext2fs_flush2( e2fs, EXT2_FLAG_FLUSH_NO_SYNC );
    if( err ) {
      ret = translate_error( e2fs, file->ino, err );
    }
  }

  fi->fh = 0;

  return 0;
}

}