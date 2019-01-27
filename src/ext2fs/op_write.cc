#include "ext2fs/fuse_ext2_int.h"

namespace medium {

static int fs_can_allocate( ext2_filsys fs, blk64_t num )
{
  blk64_t reserved;

  /*
  dbg_printf( "%s: Asking for %llu; alloc_all=%d total=%llu free=%llu "
              "rsvd=%llu\n", __func__, num, ff->alloc_all_blocks,
              ext2fs_blocks_count( fs->super ),
              ext2fs_free_blocks_count( fs->super ),
              ext2fs_r_blocks_count( fs->super ) );
  */

  if( num > ext2fs_blocks_count( fs->super ) ) {
    return 0;
  }

  /*
   * Different meaning for r_blocks -- libext2fs has bugs where the FS
   * can get corrupted if it totally runs out of blocks.  Avoid this
   * by refusing to allocate any of the reserve blocks to anybody.
   */
  reserved = ext2fs_r_blocks_count( fs->super );
  if( reserved == 0 ) {
    reserved = ext2fs_blocks_count( fs->super ) / 10;
  }
  return ext2fs_free_blocks_count( fs->super ) > reserved + num;
}

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