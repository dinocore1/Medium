#include "ext2fs/fuse_ext2.h"

namespace medium {

int Ext2FS::op_read( const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi )
{
  __u64 pos;
  errcode_t rc;
  unsigned int bytes;
  ext2_file_t efile = EXT2FS_FILE( fi->fh );

  LOG_INFO( LOG_TAG, "path = %s", path );

  //efile = do_open(e2fs, path, O_RDONLY);
  rc = ext2fs_file_llseek( efile, offset, SEEK_SET, &pos );
  if( rc ) {
    do_release( efile );
    return -EINVAL;
  }

  rc = ext2fs_file_read( efile, buf, size, &bytes );
  if( rc ) {
    do_release( efile );
    return -EIO;
  }
  //do_release(efile);


  return bytes;
}
}