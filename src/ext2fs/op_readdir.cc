#include "ext2fs/fuse_ext2.h"

#include <fuse.h>

namespace medium {

struct dir_walk_data {
	void *buf;
	fuse_fill_dir_t filler;
};

static int walk_dir (struct ext2_dir_entry *de, int offset, int blocksize, char *buf, void *priv_data)
{
	int ret;
	size_t flen;
	char *fname;
	struct dir_walk_data *b = (struct dir_walk_data*) priv_data;

	flen = de->name_len & 0xff;
	fname = (char *) malloc(sizeof(char) * (flen + 1));
	if (fname == NULL) {
		LOG_ERROR("", "s = (char *) malloc(sizeof(char) * (%d + 1)); failed", flen);
		return -ENOMEM;
	}
	snprintf(fname, flen + 1, "%s", de->name);
	LOG_DEBUG("", "b->filler(b->buf, %s, NULL, 0);", fname);
	ret = b->filler(b->buf, fname, NULL, 0);
	free(fname);
	
	return ret;
}


int Ext2FS::op_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	int rt;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;
	struct dir_walk_data dwd= {
		.buf = buf,
		.filler = filler
    };

	
	LOG_DEBUG(LOG_TAG, "op_readdir(%s, %d)", path, offset);
	
	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		LOG_DEBUG(LOG_TAG, "do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}

    rc = ext2fs_dir_iterate(e2fs, ino, 0, NULL, walk_dir, &dwd);

	if (rc) {
		LOG_DEBUG(LOG_TAG, "Error while trying to ext2fs_dir_iterate %s", path);
		return -EIO;
	}

	return 0;
}

}