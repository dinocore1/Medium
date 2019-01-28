#ifndef FUSE_EXT2_H_
#define FUSE_EXT2_H_

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <ext2fs/ext2fs.h>

#include <baseline/Baseline.h>
#include <baseline/RefBase.h>

namespace medium {

class Ext2FS
{
public:

  int open( const char* filepath, int flags );

  int op_init();
  int op_destroy();
  int op_getattr( const char* path, struct stat* stbuf );
  int op_readdir( const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi );
  int op_mkdir( const char* path, mode_t mode );
  int op_open( const char* path, struct fuse_file_info* fi );
  int op_release( const char* path, struct fuse_file_info* fi );
  int op_create( const char* path, mode_t mode, struct fuse_file_info* fi );
  int op_read( const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi );
  int op_write( const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi );
  int op_truncate( const char* path, off_t length );
  int op_ftruncate( const char* path, off_t length, struct fuse_file_info* fi );
  int op_unlink( const char* path );
  int op_rmdir( const char* path );

protected:

  struct FileHandle : public baseline::RefBase {
    ext2_ino_t ino;
    int open_flags;
    ext2_file_t efile;
  };

  static int __translate_error( ext2_filsys fs, errcode_t err, ext2_ino_t ino,
                                const char* file, int line );

  static int do_check( const char* path );
  static int do_check_split( const char* path, char** dirname, char** basename );
  static void free_split( char* dirname, char* basename );

  static int do_readinode( ext2_filsys e2fs, const char* path, ext2_ino_t* ino, struct ext2_inode* inode );
  static int do_writeinode( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode );

  static void do_fillstatbuf( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode, struct stat* st );

  static int fs_can_allocate( ext2_filsys fs, blk64_t num );
  static int check_inum_access( ext2_filsys fs, ext2_ino_t ino, mode_t mask );
  static int update_ctime( ext2_filsys fs, ext2_ino_t ino, struct ext2_inode_large* pinode );
  static int update_atime( ext2_filsys fs, ext2_ino_t ino );
  static int update_mtime( ext2_filsys fs, ext2_ino_t ino, struct ext2_inode_large* pinode );
  static void init_times( struct ext2_inode_large* inode );
  static int unlink_file_by_name( ext2_filsys fs, const char* path );
  static int remove_inode( ext2_filsys e2fs, ext2_ino_t ino );

  static const char* LOG_TAG;
  struct struct_ext2_filsys filsys;
  ext2_filsys e2fs;


};

}

#endif // FUSE_EXT2_H_