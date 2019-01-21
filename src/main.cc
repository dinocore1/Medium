#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <baseline/Baseline.h>

static int do_getattr( const char *path, struct stat *st )
{
    LOG_INFO("", "go_getattr(%s)", path);
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
    LOG_INFO("", "do_readdir(%s)", path);
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
    LOG_INFO("", "do_read(%s)", path);
}

static struct fuse_operations operations;

int main(int argc, char *argv[])
{
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
    operations.read = do_read;

    return fuse_main( argc, argv, &operations, NULL );

}