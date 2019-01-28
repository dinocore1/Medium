
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
  if( !mWindow.empty() ) {
    mOutput->close();
    mCallback->onNewBlock( mStrongHash->finalize() );
    mOutput.reset();
  }
  if( mOutput.get() ) {
    mOutput->close();
    mOutput.reset();
  }
}

int DataSplitterOutputStream::write( uint8_t* buf, size_t off, size_t len )
{

  for( int i = 0; i < len; i++ ) {
    uint8_t in = buf[i + off];
    if( mWindow.size() < WINDOW_SIZE ) {
      mRollingHash.eat( in );
    } else {
      uint8_t out = mWindow.get();
      mRollingHash.update( in, out );
    }
    mWindow.put( in );
    mStrongHash->update( &in, 1 );

    if( mOutput.get() == nullptr ) {
      mOutput.reset( mCallback->createOutput() );
    }
    mOutput->write( &in, i, 1 );

    RabinKarpHash::hashvalue_t rollingHash = mRollingHash.hash();
    if( rollingHash <= DIFFICULTY ) {
      mOutput->close();
      mCallback->onNewBlock( mStrongHash->finalize() );

      mWindow.clear();
      mStrongHash->reset();
    }
  }

  return len;
}

}