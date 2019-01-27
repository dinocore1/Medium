#include "ext2fs/fuse_ext2.h"

namespace medium {

void Ext2FS::do_fillstatbuf (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode, struct stat *st)
{
	
	memset(st, 0, sizeof(*st));
	/* XXX workaround
	 * should be unique and != existing devices */
	st->st_dev = (dev_t) ((long) e2fs);
	st->st_ino = ino;
	st->st_mode = inode->i_mode;
	st->st_nlink = inode->i_links_count;

    st->st_uid = getuid();
	st->st_gid = getgid();
    /*
	st->st_uid = ext2_read_uid(inode);
	st->st_gid = ext2_read_gid(inode);
    */

    if(S_ISBLK(inode->i_mode)) {
        //TODO redirect to the peices
    }

	st->st_rdev = 0;
	st->st_size = EXT2_I_SIZE(inode);
	st->st_blksize = EXT2_BLOCK_SIZE(e2fs->super);
	st->st_blocks = inode->i_blocks;
	st->st_atime = inode->i_atime;
	st->st_mtime = inode->i_mtime;
	st->st_ctime = inode->i_ctime;
#if __FreeBSD__ == 10
	st->st_gen = inode->i_generation;
#endif
	
}

int Ext2FS::op_getattr (const char *path, struct stat *stbuf)
{
	int rt;
	ext2_ino_t ino;
	struct ext2_inode inode;

	
	LOG_INFO(LOG_TAG, "getattr(%s)", path);

	rt = do_check(path);
	if (rt != 0) {
		LOG_DEBUG(LOG_TAG, "do_check(%s); failed", path);
		return rt;
	}

	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		LOG_DEBUG(LOG_TAG, "do_readinode(%s, &ino, &vnode); failed", path);
		return rt;
	}
	do_fillstatbuf(e2fs, ino, &inode, stbuf);

	LOG_DEBUG(LOG_TAG, "path: %s, size: %d", path, stbuf->st_size);
	
	return 0;
}

}