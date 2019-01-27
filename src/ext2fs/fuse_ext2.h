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


class Ext2FS {
public:

    int open(const char* filepath, int flags);

    int op_getattr (const char *path, struct stat *stbuf);
    int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
    int op_mkdir (const char *path, mode_t mode);
    int op_open(const char *path, struct fuse_file_info *fi);
    int op_release (const char *path, struct fuse_file_info *fi);

private:
    static int do_check (const char *path);
    static int do_check_split (const char *path, char **dirname, char **basename);
    static void free_split(char* dirname, char* basename);

    static int do_readinode (ext2_filsys e2fs, const char *path, ext2_ino_t *ino, struct ext2_inode *inode);
    static int do_writeinode (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode);

    static void do_fillstatbuf(ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode, struct stat *st);

    static ext2_file_t do_open (ext2_filsys e2fs, const char *path, int flags);
    static int do_release(ext2_file_t efile);

    static const char* LOG_TAG;
    struct struct_ext2_filsys filsys;
    ext2_filsys e2fs;
};

}

#endif // FUSE_EXT2_H_