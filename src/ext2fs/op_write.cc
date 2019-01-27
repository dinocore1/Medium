#include "ext2fs/fuse_ext2.h"

namespace medium {

size_t Ext2FS::do_write( ext2_file_t efile, const char* buf, size_t size, off_t offset )
{
  int rt;
  __u64 fsize;
  rt = ext2fs_file_get_lsize( efile, &fsize );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_file_get_lsize(efile, &fsize); failed" );
    return -EIO;
  }

  rt = ext2fs_file_llseek( efile, offset, SEEK_SET, NULL );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_file_lseek(efile, %lld, SEEK_SET, &npos); failed", offset );
    return rt;
  }

  size_t bytesLeft = size;
  size_t i = 0;
  unsigned int bytesWritten;
  while( bytesLeft > 0 ) {
    rt = ext2fs_file_write( efile, &buf[i], bytesLeft, &bytesWritten );
    if( rt != 0 ) {
      LOG_ERROR( LOG_TAG, "write error: 0x%x", rt );
      return -EIO;
      //return rt;
    }
    i += bytesWritten;
    bytesLeft -= bytesWritten;
  }

  if( offset + bytesWritten > fsize ) {
    rt = ext2fs_file_set_size2( efile, offset + bytesWritten );
    if( rt ) {
      LOG_DEBUG( LOG_TAG, "extfs_file_set_size(efile, %lld); failed", offset + bytesWritten );
      return -EIO;
    }
  }

  rt = ext2fs_file_flush( efile );
  if( rt != 0 && rt != EXT2_ET_BLOCK_ALLOC_FAIL ) {
    LOG_DEBUG( LOG_TAG, "ext2_file_flush(efile); failed" );
    return -EIO;
  }

  return bytesWritten;
}

int Ext2FS::op_write( const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi )
{
  size_t rt;
  ext2_file_t efile = EXT2FS_FILE( fi->fh );


  LOG_INFO( LOG_TAG, "path = %s", path );

  //efile = do_open(e2fs, path, O_WRONLY);

  rt = do_write( efile, buf, size, offset );

  //do_release(efile);

  return rt;
}

}