
#include <db_cxx.h>

#include <baseline/Baseline.h>
#include <baseline/Mutex.h>

#include "Filesystem.h"

using baseline::Mutex;

namespace medium {

int BlockStorage::read( uint64_t offset, uint8_t* buf, size_t len )
{
  size_t i;
  BlockStorage::Info info;
  this->info( info );
  const size_t blockSize = info.blocksize_kb * 1024;

  uint32_t blockid = offset / blockSize;
  uint32_t blockoffset = offset % blockSize;

  uint8_t* blockBuf = ( uint8_t* ) malloc( blockSize );

  i = 0;
  while( len > 0 ) {
    readBlock( blockid++, blockBuf, 0 );
    size_t numBytes = MIN( len, blockSize - blockoffset );
    memcpy( &buf[i], &blockBuf[blockoffset], numBytes );
    len -= numBytes;
    i += numBytes;
    blockoffset = 0;
  }

  free( blockBuf );

}

int BlockStorage::write( uint64_t offset, uint8_t* buf, size_t len )
{
  size_t i;
  Info info;
  this->info( info );
  const size_t blockSize = info.blocksize_kb * 1024;

  uint32_t blockid = offset / blockSize;
  uint32_t blockoffset = offset % blockSize;

  uint8_t* blockBuf = ( uint8_t* ) malloc( blockSize );

  i = 0;
  while( len > 0 ) {
    readBlock( blockid, blockBuf, 0 );
    size_t numBytes = MIN( len, blockSize - blockoffset );
    memcpy( &blockBuf[blockoffset], &buf[i], numBytes );
    writeBlock( blockid, blockBuf, 0 );
    len -= numBytes;
    i += numBytes;
    blockoffset = 0;
    blockid += 1;
  }

  free( blockBuf );

}

#define FILEBLOCK_TAG "FileBlockStorage"
#define FBS_BLOCK_SIZE 4096

static const char FBS_Magic[] = { 'F', 'B', 'S', 'v', '1', '0', '0', '0' };

struct FileBlockStorageHeader {
  char magic[8];
  uint8_t reserved[8];
  uint32_t numBlocks;
};

int FileBlockStorage::open( const char* path, FileBlockStorage& fbs, uint32_t& numBlocks )
{
  int err;
  bool alreadyExists;
  FileBlockStorageHeader header;

  fbs.mFD = fopen( path, "r+" );
  if( fbs.mFD != NULL ) {
    if( ( err = fread( &header, sizeof( FileBlockStorageHeader ), 1, fbs.mFD ) ) != 1 ) {
      LOG_ERROR( FILEBLOCK_TAG, "could not read header" );
      return err;
    }

    if( memcmp( FBS_Magic, header.magic, sizeof( FBS_Magic ) ) != 0 ) {
      LOG_ERROR( FILEBLOCK_TAG, "wrong header magic" );
      return -1;
    }
    numBlocks = header.numBlocks;
    fbs.mNumBlocks = header.numBlocks;

  } else {
    //new file creation and format
    fbs.mFD = fopen( path, "w+" );
    if( fbs.mFD == NULL ) {
      LOG_ERROR( FILEBLOCK_TAG, "could not create storage file" );
      return -1;
    }
    memcpy( header.magic, FBS_Magic, sizeof( FBS_Magic ) );
    header.numBlocks = numBlocks;

    err = fwrite( &header, sizeof( FileBlockStorageHeader ), 1, fbs.mFD );
    void* buf = malloc( FBS_BLOCK_SIZE );
    memset( buf, 0, FBS_BLOCK_SIZE );

    for( int i = 0; i < numBlocks; i++ ) {
      if( ( err = fwrite( buf, FBS_BLOCK_SIZE, 1, fbs.mFD ) ) != 1 ) {
        LOG_ERROR( FILEBLOCK_TAG, "could not write to storage file: 0x%x", errno );
        return -1;
      }
    }

    free( buf );
    fbs.mNumBlocks = numBlocks;

  }

  return 0;
}

void FileBlockStorage::info( Info& info )
{
  info.blocksize_kb = 4;
  info.numBlocks = mNumBlocks;
}

void FileBlockStorage::close()
{
  fclose( mFD );
  mFD = nullptr;
}

int FileBlockStorage::readBlock( uint32_t id, uint8_t* buf, size_t offset )
{
  size_t err;
  long f_offset = sizeof( FileBlockStorageHeader ) + ( id * FBS_BLOCK_SIZE );
  fseek( mFD, f_offset, SEEK_SET );
  if( ( err = fread( buf, sizeof( uint8_t ), FBS_BLOCK_SIZE, mFD ) ) != FBS_BLOCK_SIZE ) {
    LOG_ERROR( FILEBLOCK_TAG, "error reading block: %d", id );
    return -1;
  }
  return 0;
}

int FileBlockStorage::writeBlock( uint32_t id, uint8_t* buf, size_t offset )
{
  size_t err;
  long f_offset = sizeof( FileBlockStorageHeader ) + ( id * FBS_BLOCK_SIZE );
  fseek( mFD, f_offset, SEEK_SET );
  if( ( err = fwrite( buf, sizeof( uint8_t ), FBS_BLOCK_SIZE, mFD ) ) != FBS_BLOCK_SIZE ) {
    LOG_ERROR( FILEBLOCK_TAG, "error writing block: %d", id );
    return -1;
  }
  return 0;
}

int FileBlockStorage::read( uint64_t offset, uint8_t* buf, size_t len )
{
  long f_offset = sizeof( FileBlockStorageHeader ) + offset;
  fseek( mFD, f_offset, SEEK_SET );
  fread( buf, 1, len, mFD );
}

int FileBlockStorage::write( uint64_t offset, uint8_t* buf, size_t len )
{
  long f_offset = sizeof( FileBlockStorageHeader ) + offset;
  fseek( mFD, f_offset, SEEK_SET );
  fwrite( buf, 1, len, mFD );
}

////////////////////////// FileSystem ////////////////////////////////

static
const uint8_t FS_Magic[] = {'m', 'e', 'd'};

#define FS_TAG "FS"

FileSystem::FileSystem( BlockStorage* bs )
  : mBlockStorage( bs )
{}

int FileSystem::open()
{
  BlockStorage::Info info;
  mBlockStorage->info( info );
  uint8_t* buf = ( uint8_t* ) malloc( info.blocksize_kb * 1024 );
  mBlockStorage->readBlock( 0, buf, 0 );

  memcpy( &mSuperblock, buf, sizeof( SuperBlock ) );

  free( buf );

  if( memcmp( mSuperblock.magic, FS_Magic, sizeof( FS_Magic ) ) != 0 ) {
    LOG_ERROR( FS_TAG, "wrong FS magic" );
    return -1;
  }

  mBlockBuffer = ( uint8_t* ) malloc( mSuperblock.block_k_size * 1024 );

  return 0;
}

int FileSystem::close()
{
  if( mBlockBuffer != nullptr ) {
    free( mBlockBuffer );
    mBlockBuffer = nullptr;
  }
  return 0;
}

#define ROUND_UP(x, y) (((x) + (y)-1)) / (y)

int FileSystem::format()
{
  BlockStorage::Info info;
  mBlockStorage->info( info );
  const uint64_t storageSize = info.blocksize_kb * 1024 * info.numBlocks;

  memcpy( mSuperblock.magic, FS_Magic, sizeof( uint8_t ) * 3 );
  mSuperblock.block_k_size = info.blocksize_kb;
  const uint32_t inodes_per_block = ( mSuperblock.block_k_size * 1024 ) / sizeof( Inode );
  mSuperblock.blocks_per_group = 8 * 1024;
  uint32_t inode_blocks_per_group = mSuperblock.blocks_per_group / 10;
  uint32_t data_blocks_per_group = mSuperblock.blocks_per_group - inode_blocks_per_group - 3;
  mSuperblock.inodes_per_group = inode_blocks_per_group * inodes_per_block;

  mSuperblock.num_groups = info.numBlocks / mSuperblock.blocks_per_group;
  uint32_t leftOverBlocks = info.numBlocks % mSuperblock.blocks_per_group;
  uint32_t leftover_inode_blocks = leftOverBlocks / 10;
  uint32_t leftover_data_blocks;
  if( leftover_inode_blocks > 4 ) {
    leftover_data_blocks = leftOverBlocks - leftover_inode_blocks - 3;
  } else {
    leftover_data_blocks = 0;
  }

  mSuperblock.num_blocks = ( mSuperblock.num_groups * data_blocks_per_group ) + ( leftover_data_blocks );
  mSuperblock.num_inodes = ( mSuperblock.num_groups * inode_blocks_per_group ) + ( leftover_inode_blocks * inodes_per_block );

  mBlockBuffer = ( uint8_t* ) malloc( mSuperblock.block_k_size * 1024 );

  //write superblock
  memcpy( mBlockBuffer, &mSuperblock, sizeof( mSuperblock ) );
  mBlockStorage->writeBlock( 0, mBlockBuffer, 0 );

  for( uint32_t i = 0; i < ROUND_UP( info.numBlocks, mSuperblock.blocks_per_group ); i++ ) {
    BlockGroup bg;
    bg.numBlocks = mSuperblock.blocks_per_group;
    bg.numINodes = mSuperblock.inodes_per_group;
    memcpy( mBlockBuffer, &bg, sizeof( bg ) );
    mBlockStorage->writeBlock( i * mSuperblock.blocks_per_group + 1, mBlockBuffer, 0 );

    //write inode bitmap
    memset( mBlockBuffer, 0, mSuperblock.block_k_size * 1024 );
    mBlockStorage->writeBlock( i * mSuperblock.blocks_per_group + 2, mBlockBuffer, 0 );

    //write bitmap for blocks
    memset( mBlockBuffer, 0, mSuperblock.block_k_size * 1024 );
    mBlockStorage->writeBlock( i * mSuperblock.blocks_per_group + 3, mBlockBuffer, 0 );
  }

  //write root dir entry
  //alloc first inode should alway be zero
  memset( mBlockBuffer, 0, mSuperblock.block_k_size * 1024 );
  mBlockBuffer[0] = 0x80;
  mBlockStorage->writeBlock( 2, mBlockBuffer, 0 );

  Inode inode;
  inode.size = 0;
  inode.file_type = IN_TYPE_DIR;
  inode.links = 2;
  memset( inode.blocks, 0, sizeof( blockid_t ) * 15 );
  allocBlock( 0, inode.blocks[0] );
  writeInode( inode, 0 );

}

static
int findFirstFree( uint8_t* fat, const uint16_t numItems )
{
  const uint16_t numBytes = ROUND_UP( numItems, 8 );
  for( int i = 0; i < numBytes; i++ ) {
    uint8_t value = fat[i];
    if( value < 0xFF ) {
      for( int j = 0; j < 8; j++ ) {
        if( value < ( 1 << j ) ) {
          return 8 * i + j;
        }
      }
    }
  }
  return -1;
}

int FileSystem::allocInode( uint32_t blockGroup, blockid_t& inodeId )
{
  int ret = -1;
  Mutex::Autolock lock( mWriteLock );
  uint32_t fatBlockId = ( blockGroup * mSuperblock.inodes_per_group ) + 2;
  mBlockStorage->readBlock( fatBlockId, mBlockBuffer, 0 );

  int offset = findFirstFree( mBlockBuffer, mSuperblock.inodes_per_group );
  if( offset >= 0 ) {
    //mark used
    uint8_t value = mBlockBuffer[offset / 8];
    uint16_t bit = 1 << ( offset % 8 );
    value |= bit;
    mBlockBuffer[offset / 8] = value;
    mBlockStorage->writeBlock( fatBlockId, mBlockBuffer, 0 );
    inodeId = fatBlockId + offset;
    return 0;
  }

  return -1;
}

int FileSystem::allocBlock( uint32_t blockGroup, blockid_t& blockId )
{
  int ret = -1;
  Mutex::Autolock lock( mWriteLock );
  uint32_t fatBlockId = ( blockGroup * mSuperblock.blocks_per_group ) + 3;
  mBlockStorage->readBlock( fatBlockId, mBlockBuffer, 0 );

  int offset = findFirstFree( mBlockBuffer, mSuperblock.blocks_per_group );
  if( offset >= 0 ) {
    //mark used
    uint8_t value = mBlockBuffer[offset / 8];
    uint16_t bit = 1 << ( offset % 8 );
    value |= bit;
    mBlockBuffer[offset / 8] = value;
    mBlockStorage->writeBlock( fatBlockId, mBlockBuffer, 0 );
    blockId = fatBlockId + offset;
    return 0;
  }

  return -1;
}

int FileSystem::writeInode( const Inode& inode, blockid_t id )
{
  const uint32_t inodes_per_block = ( mSuperblock.block_k_size * 1024 ) / sizeof( Inode );

  Mutex::Autolock lock( mWriteLock );

  uint32_t blockGroup = id / mSuperblock.inodes_per_group;
  uint32_t blockId = ( blockGroup * mSuperblock.blocks_per_group ) + 4 + ( id % inodes_per_block );
  uint32_t tableIdx = id % mSuperblock.inodes_per_group;

  mBlockStorage->readBlock( blockId, mBlockBuffer, 0 );
  memcpy( &mBlockBuffer[sizeof( Inode ) * tableIdx], &inode, sizeof( Inode ) );
  mBlockStorage->writeBlock( blockId, mBlockBuffer, 0 );

}

} // namespace