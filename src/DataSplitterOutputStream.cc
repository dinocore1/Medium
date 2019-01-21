
#include <baseline/Baseline.h>

#include "RabinKarp.h"
#include "DataSplitterOutputStream.h"

namespace medium {

#define WINDOW_SIZE 16
#define DIFFICULTY 0x00000FFF

//should be 1 split on average in 1 MiB

DataSplitterOutpuStream::DataSplitterOutpuStream( Callback* cb )
  : mOnNewStream( cb ), mRollingHash( WINDOW_SIZE ), mWindow( WINDOW_SIZE ), mOutput( nullptr )
{}

void DataSplitterOutpuStream::close()
{
  if( mOutput ) {
    mOutput->close();
    mOutput = nullptr;
  }
}

int DataSplitterOutpuStream::write( uint8_t* buf, size_t off, size_t len )
{
  if( mOutput == nullptr ) {
    mOutput = mOnNewStream->createOutput();
  }
  for( int i = 0; i < len; i++ ) {
    uint8_t in = buf[i];
    if( mWindow.size() < WINDOW_SIZE ) {
      mRollingHash.eat( in );
    } else {
      uint8_t out = mWindow.get();
      mRollingHash.update( in, out );
    }
    mWindow.put( in );

    RabinKarpHash::hashvalue_t rollingHash = mRollingHash.hash();
    if( rollingHash <= DIFFICULTY ) {
      mOutput->close();
      mOutput = mOnNewStream->createOutput();
    }

    mOutput->write( buf, i, 1 );
  }

  return len;
}

}