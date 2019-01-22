
#include <baseline/Baseline.h>

#include "RabinKarp.h"
#include "DataSplitterOutputStream.h"

namespace medium {

#define WINDOW_SIZE 16
#define DIFFICULTY 0x00000FFF

//should be 1 split on average in 1 MiB

DataSplitterOutputStream::DataSplitterOutputStream( Callback* cb )
  : mCallback( cb ), mRollingHash( WINDOW_SIZE ), mWindow( WINDOW_SIZE ), mOutput( nullptr ), mStrongHash( createSHA1() )
{}

void DataSplitterOutputStream::close()
{
  if( mOutput ) {
    mOutput->close();
    mOutput = nullptr;
  }
}

int DataSplitterOutputStream::write( uint8_t* buf, size_t off, size_t len )
{
  if( mOutput == nullptr ) {
    mOutput = mCallback->createOutput();
    mStrongHash->reset();
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
    mStrongHash->update( &buf[i], 1 );

    RabinKarpHash::hashvalue_t rollingHash = mRollingHash.hash();
    if( rollingHash <= DIFFICULTY ) {
      mOutput->close();
      mCallback->onNewBlock( mStrongHash->finalize() );
      mStrongHash->reset();
      mOutput = mCallback->createOutput();
    }

    mOutput->write( buf, i, 1 );
  }

  return len;
}

}