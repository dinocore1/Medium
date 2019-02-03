#include "ScanTask.h"

#define LOG_TAG "ScanTask"

using namespace baseline;

namespace medium {


class Ex2FileOutputStream : public OutputStream
{
public:
  Ex2FileOutputStream( Ext2FS& fs )
    : mFS( fs ) {
    int err;
    fi.flags = O_WRONLY;

    err = mFS.op_open( "/incoming.dat", &fi );
    if( err ) {
      err = mFS.op_create( "/incoming.dat", S_IFREG | 0644, &fi );
      if( err ) {
        LOG_ERROR( LOG_TAG, "could not create new file: %d", err );
        return;
      }
    }

    Ext2FS::FileHandle* file = ( Ext2FS::FileHandle* ) fi.fh;
    ino = file->ino;

    err = ext2fs_file_set_size2( file->efile, 0 );


  }

  void close() {
    mFS.op_release( nullptr, &fi );
  }

  int write( uint8_t* buf, size_t offset, size_t len ) {
    unsigned int bytes_written;
    Ext2FS::FileHandle* file = ( Ext2FS::FileHandle* ) fi.fh;
    int i = 0;
    errcode_t err;

    while( len > 0 ) {
      err = ext2fs_file_write( file->efile, &buf[i], len, &bytes_written );
      if( err ) {
        LOG_ERROR( LOG_TAG, "" );
        return -1;
      }
      i += bytes_written;
      len -= bytes_written;
    }

    return 0;
  }

private:
  Ext2FS& mFS;
  struct fuse_file_info fi;
  ext2_ino_t ino;
};

ScanTask::ScanTask( Ext2FS& liveFS, ext2_ino_t ino, Ext2FS& partsFS )
  : mLiveFS( liveFS ), mIno( ino ), mPartsFS( partsFS )
{}

#define BUF_SIZE 1024

void ScanTask::run()
{
  ext2_file_t efile;
  errcode_t err;
  uint8_t buf[BUF_SIZE];
  __u64 filelen;
  unsigned int bytes_read;



  LOG_INFO( LOG_TAG, "starting scan ino=%d", mIno );


  err = ext2fs_file_open( mLiveFS.e2fs, mIno, 0, &efile );
  if( err ) {
    LOG_ERROR( LOG_TAG, "error ext2fs_file_open: 0x%x", err );
    return;
  }

  err = ext2fs_file_get_lsize( efile, &filelen );
  if( err ) {
    LOG_ERROR( LOG_TAG, "error ext2fs_file_get_lsize" );
    return;
  }

  err = ext2fs_file_llseek( efile, 0, SEEK_SET, NULL );
  if( err ) {
    LOG_ERROR( LOG_TAG, "error ext2fs_file_llseek" );
    return;
  }


  DataSplitterOutputStream dout( this );

  while( filelen > 0 ) {
    err = ext2fs_file_read( efile, buf, MIN( BUF_SIZE, filelen ), &bytes_read );
    if( err ) {
      LOG_ERROR( LOG_TAG, "read error: %d", err );
      return;
    }
    filelen -= bytes_read;

    dout.write( buf, 0, bytes_read );

    LOG_INFO(LOG_TAG, "%d bytes left", filelen);
  }

  dout.close();

  LOG_INFO( LOG_TAG, "done" );


}

OutputStream* ScanTask::createOutput()
{
  return new Ex2FileOutputStream( mPartsFS );
}

void ScanTask::onNewBlock( const HashCode& code )
{
  LOG_INFO( LOG_TAG, "hash peice: %s", code.toHexString().string() );
}

} // medium
