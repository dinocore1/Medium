#include "ext2fs/fuse_ext2.h"

namespace medium {

static int release_blocks_proc( ext2_filsys fs, blk_t* blocknr, int blockcnt, void* private_data )
{
  blk_t block;

  block = *blocknr;
  ext2fs_block_alloc_stats( fs, block, -1 );

#ifdef CLEAN_UNUSED_BLOCKS
  *blocknr = 0;
  return BLOCK_CHANGED;
#else
  return 0;
#endif
}

int Ext2FS::do_killfilebyinode( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode )
{
  errcode_t rc;
  char scratchbuf[3 * e2fs->blocksize];

  inode->i_links_count = 0;
  inode->i_dtime = time( NULL );

  rc = ext2fs_write_inode( e2fs, ino, inode );
  if( rc ) {
    LOG_DEBUG( LOG_TAG, "ext2fs_write_inode(e2fs, ino, inode); failed" );
    return -EIO;
  }

  if( ext2fs_inode_has_valid_blocks( inode ) ) {
    LOG_DEBUG( LOG_TAG, "start block delete for %d", ino );
#ifdef CLEAN_UNUSED_BLOCKS
    ext2fs_block_iterate( e2fs, ino, BLOCK_FLAG_DEPTH_TRAVERSE, scratchbuf, release_blocks_proc, NULL );
#else
    ext2fs_block_iterate( e2fs, ino, 0, scratchbuf, release_blocks_proc, NULL );
#endif
  }

  ext2fs_inode_alloc_stats2( e2fs, ino, -1, LINUX_S_ISDIR( inode->i_mode ) );


  return 0;
}

} // medium
