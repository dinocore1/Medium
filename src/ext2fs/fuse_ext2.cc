#include "ext2fs/fuse_ext2.h"

namespace medium {


const char* Ext2FS::LOG_TAG = "Ext2FS";


int Ext2FS::open(const char* filepath, int flags)
{
    int err;
    e2fs = &filsys;
    err = ext2fs_open(filepath, flags, 0, 0, unix_io_manager, &e2fs);
    if(err != 0) {
        LOG_ERROR(LOG_TAG, "could not open ext2 image: %s", filepath);
        return err;
    }

    err = ext2fs_read_bitmaps(e2fs);
    if(err != 0) {
        LOG_ERROR(LOG_TAG, "error reading bitmaps: %d", err);
    }

    return err;
}

}