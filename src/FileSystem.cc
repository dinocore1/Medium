
#include <db_cxx.h>

#include <baseline/Baseline.h>
#include <baseline/Mutex.h>

#include "Filesystem.h"

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

FileSystem::FileSystem( BlockStorage* bs )
  : mBlockStorage( bs )
{}

void FileSystem::open()
{}

} // namespace