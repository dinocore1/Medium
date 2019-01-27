#include "ext2fs/fuse_ext2_int.h"

namespace medium {

ext2_file_t Ext2FS::do_open( ext2_filsys e2fs, const char* path, int flags )
{
  int rt;
  errcode_t rc;
  ext2_ino_t ino;
  ext2_file_t efile;
  struct ext2_inode inode;
  //struct fuse_context *cntx = fuse_get_context();
  //struct extfs_data *e2data = cntx->private_data;

  LOG_DEBUG( LOG_TAG, "path = %s", path );

  rt = do_check( path );
  if( rt != 0 ) {
    LOG_DEBUG( LOG_TAG, "do_check(%s); failed", path );
    return NULL;
  }

  rt = do_readinode( e2fs, path, &ino, &inode );
  if( rt ) {
    LOG_DEBUG( LOG_TAG, "do_readinode(%s, &ino, &inode); failed", path );
    return NULL;
  }

  int ext2_flags = 0;
  //if( flags & O_CREAT ) {
  //  ext2_flags |= EXT2_FILE_CREATE;
  //}

  if( flags & O_WRONLY || flags & O_RDWR ) {
    ext2_flags |= EXT2_FILE_WRITE;
  }

  rc = ext2fs_file_open2(
         e2fs,
         ino,
         &inode,
         ext2_flags,
         &efile );
  if( rc ) {
    return NULL;
  }

  //TODO: take a look at this
  /*
  if (e2data->readonly == 0) {
  inode.i_atime = e2fs->now ? e2fs->now : time(NULL);
  rt = do_writeinode(e2fs, ino, &inode);
  if (rt) {
  	LOG_DEBUG(LOG_TAG, "do_writeinode(%s, &ino, &inode); failed", path);
  	return NULL;
  }
  }
  */



  return efile;
}

int Ext2FS::op_open( const char* path, struct fuse_file_info* fi )
{
  ext2_file_t efile;

  LOG_INFO( LOG_TAG, "path = %s", path );

  efile = do_open( e2fs, path, fi->flags );
  if( efile == NULL ) {
    LOG_DEBUG( LOG_TAG, "do_open(%s); failed", path );
    return -ENOENT;
  }
  fi->fh = ( uint64_t ) efile;


  return 0;
}


}