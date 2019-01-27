#include "ext2fs/fuse_ext2.h"

#define translate_error(fs, ino, err) __translate_error((fs), (err), (ino), \
    __FILE__, __LINE__)

