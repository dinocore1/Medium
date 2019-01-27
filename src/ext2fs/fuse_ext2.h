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

namespace medium {

#define EXT2FS_FILE(efile) ((ext2_file_t) (unsigned long) (efile))

class Ext2FS
{
public:

  int open( const char* filepath, int flags );

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

private:

#define translate_error(fs, ino, err) __translate_error((fs), (err), (ino), \
    __FILE__, __LINE__)

  static int __translate_error( ext2_filsys fs, errcode_t err, ext2_ino_t ino,
                                const char* file, int line );

  static int do_check( const char* path );
  static int do_check_split( const char* path, char** dirname, char** basename );
  static void free_split( char* dirname, char* basename );

  static int do_readinode( ext2_filsys e2fs, const char* path, ext2_ino_t* ino, struct ext2_inode* inode );
  static int do_writeinode( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode );

  static void do_fillstatbuf( ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode* inode, struct stat* st );

  static ext2_file_t do_open( ext2_filsys e2fs, const char* path, int flags );
  static int do_release( ext2_file_t efile );

  static int do_create( ext2_filsys e2fs, const char* path, mode_t mode, dev_t dev, const char* fastsymlink );

  static size_t do_write( ext2_file_t efile, const char* buf, size_t size, off_t offset );

  static int do_truncate( ext2_filsys e2fs, ext2_file_t efile, const char* path, off_t length );

  static int do_check_empty_dir( ext2_filsys e2fs, ext2_ino_t ino );
  static int do_remove_inode( ext2_filsys e2fs, ext2_ino_t ino );

  static const char* LOG_TAG;
  struct struct_ext2_filsys filsys;
  ext2_filsys e2fs;


};

}

#endif // FUSE_EXT2_H_