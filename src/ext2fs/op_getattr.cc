#include "ext2fs/fuse_ext2_int.h"

namespace medium {

int Ext2FS::op_getattr( const char* path, struct stat* stbuf )
{
  int ret;
  errcode_t err;
  ext2_ino_t ino;

  LOG_INFO(LOG_TAG, "path=%s", path);

  err = ext2fs_namei(e2fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &ino);
	if (err) {
		ret = translate_error(e2fs, 0, err);
		return ret;
	}
	ret = stat_inode(e2fs, ino, stbuf);

  return ret;
}

}