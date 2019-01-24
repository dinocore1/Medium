#ifndef MEDIUM_FILESYSTE_H_
#define MEDIUM_FILESYSTE_H_

#include <cstdio>

#include <baseline/Baseline.h>
#include <baseline/Hash.h>

#include <db_cxx.h>

namespace medium {

#define MAX_FILENAME_LEN 63
#define BLOCK_SIZE 4096

struct INode {
  char name[MAX_FILENAME_LEN];
  uint64_t flags;
  DB_HEAP_RID ptrs[16];

  void setSize(uint64_t);
  uint64_t getSize();
  void setFlags(uint8_t);
  uint8_t getFlags();
};

struct SuperBlock {
  uint32_t numINodes;
  uint32_t numDataBlocks;
  uint16_t freeINodeBlock;
  uint16_t freeDataBlock;
};

class BlockStorage
{
public:

  virtual void close() = 0;
  virtual int readBlock( uint32_t id, uint8_t* buf, size_t offset ) = 0;
  virtual int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) = 0;

};


struct DataBlock {
  baseline::HashCode strongHash;
  uint32_t size;
  uint16_t flags;
};

class FileBlockStorage : public BlockStorage
{
public:

  static int open( const char* path, FileBlockStorage& );

  int format( uint32_t numBlocks );

  void close() override;
  int readBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
  int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) override;

private:
  FILE* mFD;
  uint32_t mNumBlocks;
};

class MemBlockStorage : public BlockStorage
{
public:


  void close() override;
  int readBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
  int writeBlock( uint32_t id, uint8_t* buf, size_t offset ) override;
};

class FileSystem
{

};


} // namespace

#endif // MEDIUM_FILESYSTE_H_