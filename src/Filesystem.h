#ifndef MEDIUM_FILESYSTE_H_
#define MEDIUM_FILESYSTE_H_

#include <cstdio>

#include <baseline/Baseline.h>
#include <baseline/Hash.h>
#include <baseline/Mutex.h>

#include <db_cxx.h>

namespace medium {

typedef uint32_t blockid_t;

struct SuperBlock {
  uint8_t magic[3];
  uint32_t num_inodes;
  uint32_t num_blocks;
  uint8_t block_k_size;
  uint16_t inodes_per_group;
  uint16_t blocks_per_group;
  uint32_t num_groups;
};


#define DIR_READ_MASK   (1 << 0)
#define DIR_WRITE_MASK  (1 << 1)
#define DIR_EXE_MASK    (1 << 2)

#define DIR_TYPE_MASK 0x18
#define DIR_TYPE_DIR  0x0
#define DIR_TYPE_REG  0x8
#define DIR_TYPE_SYMLINK 0x18

//Simular to ext2 https://www.nongnu.org/ext2-doc/ext2.html#LINKED-DIRECTORIES
struct DirectoryEntry {
  blockid_t inode;
  uint16_t rec_len;
  uint8_t nameLen;
  uint8_t file_type;
};

struct BlockGroup {
  uint16_t numBlocks;
  uint16_t numINodes;
};

#define IN_TYPE_MASK    0xF
#define IN_TYPE_DIR     0x0
#define IN_TYPE_REG     0x1
#define IN_TYPE_REG_SEG 0x2
#define IN_TYPE_SEGMENT 0x3

struct Inode {
  uint64_t size;
  uint8_t file_type;
  uint16_t links;
  uint64_t ctime; //creation time seconds since 1970-1-1
  uint64_t mtime; //modify time seconds since 1970-1-1
  uint8_t sha1[20];
  blockid_t blocks[15];
};

class BlockStorage
{
public:

  struct Info {
    uint16_t blocksize_kb;
    uint32_t numBlocks;
  };

  virtual void close() = 0;
  virtual void info( Info& info ) = 0;
  virtual int readBlock( uint32_t id, uint8_t* buf, size_t offset ) = 0;
  virtual int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) = 0;

  virtual int read( uint64_t offset, uint8_t* buf, size_t len );
  virtual int write( uint64_t offset, uint8_t* buf, size_t len );

};


class FileBlockStorage : public BlockStorage
{
public:

  static int open( const char* path, FileBlockStorage&, uint32_t& numBlocks );

  void close() override;
  void info( Info& info );
  int readBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
  int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
  int read( uint64_t offset, uint8_t* buf, size_t len ) override;
  int write( uint64_t offset, uint8_t* buf, size_t len ) override;

private:
  FILE* mFD;
  uint32_t mNumBlocks;
};

class MemBlockStorage : public BlockStorage
{
public:

  void close() override;
  void info( Info& info );
  int readBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
  int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
};

class FileSystem
{
public:
  FileSystem( BlockStorage* );
  int open();
  int close();
  int format();

protected:

  int allocInode( uint32_t blockGroup, blockid_t& inodeId );
  int allocBlock( uint32_t blockGroup, blockid_t& blockId );
  int writeInode( const Inode&, blockid_t inodeId );

  BlockStorage* mBlockStorage;
  SuperBlock mSuperblock;
  baseline::Mutex mWriteLock;
  uint8_t* mBlockBuffer;
};


} // namespace

#endif // MEDIUM_FILESYSTE_H_