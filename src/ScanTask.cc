#include "ScanTask.h"

#define LOG_TAG "ScanTask"

using namespace baseline;

namespace medium {

ScanTask::ScanTask( Ext2FS& liveFS, ext2_ino_t ino )
  : mLiveFS( liveFS ), mIno( ino )
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
  }

  LOG_INFO( LOG_TAG, "done" );


}

OutputStream* ScanTask::createOutput()
{
  return new NullOutputStream();
}

void ScanTask::onNewBlock( const HashCode& code )
{
  LOG_INFO( LOG_TAG, "hash peice: %s", code.toHexString().string() );
}

} // medium
