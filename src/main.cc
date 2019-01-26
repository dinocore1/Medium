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


struct struct_ext2_filsys filsys;
ext2_filsys fs;

static int path_check(const char* path, bool* isend)
{
  
  for(int i=0;i<256;i++){
    switch(path[i]) {
      case '/':
        *isend = false;
        return i;

      case 0:
        *isend = true;
        return i;
    }
  }

  return 0;
}

static int get_inode(const char* path, ext2_ino_t& ino)
{
  bool isend;
  int len, err;
  ext2_ino_t inode;

  if(strcmp(path, "/") == 0){
    return 0;
  }
  
  if(path[0] != '/') {
    return -1;
  }

  path++;

  inode = fs->super->s_first_ino;

  while(true) {
    len = path_check(path, &isend);
    err = ext2fs_lookup(fs, inode, path, len, NULL, &inode);
    if(err == 0 && isend) {
      ino = inode;
      return 0;
    }
    if(err != 0) {
      //file not found
      return -1;
    }
  }

}

static int do_getattr( const char* path, struct stat* st )
{
  int err;
  ext2_ino_t ino;
  struct ext2_inode inode;

  LOG_INFO( "", "do_getattr(%s)", path );
  

  if(get_inode(path, ino) != 0) {
    return -ENOENT;
  }

  err = ext2fs_read_inode(fs, ino, &inode);
  


  return 0;
}

typedef struct dir_entry_t {
  void* buffer;
  fuse_fill_dir_t filler;
  off_t offset;
} dir_entry_t;

static int add_dir_entry(struct ext2_dir_entry *dirent,
					  int	offset,
					  int	blocksize,
					  char	*buf,
					  void	*priv_data)
{
  dir_entry_t* entry = (dir_entry_t*) priv_data;

  entry->filler(entry->buffer, dirent->name, NULL, 0);

  return 0;
}

static int do_readdir( const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi )
{
  ext2_ino_t inode;
  int retval;
  dir_entry_t ls;

  LOG_INFO( "", "do_readdir(%s)", path );

  if(get_inode(path, inode) != 0) {
    return -ENOENT;
  }

  ls.buffer = buffer;
  ls.filler = filler;
  ls.offset = offset;

  retval = ext2fs_dir_iterate(fs, inode, 0, NULL, add_dir_entry, &ls);
	return retval;
}

static int do_read( const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi )
{
  LOG_INFO( "", "do_read(%s)", path );
}

static struct fuse_operations operations;



int main( int argc, char* argv[] )
{
  int err;
  fs = &filsys;

  const char* filepath = "file.fs";

  if((err = ext2fs_open(filepath, EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs)) != 0){
    LOG_ERROR("", "could not open fs image: %s", filepath);
  }

  operations.getattr = do_getattr;
  operations.readdir = do_readdir;
  operations.read = do_read;

  return fuse_main( argc, argv, &operations, NULL );

}