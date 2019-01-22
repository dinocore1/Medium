#ifndef MEDIUM_DATASPLITTER_H_
#define MEDIUM_DATASPLITTER_H_

#include <baseline/Baseline.h>
#include <baseline/Streams.h>
#include <baseline/CircleBuffer.h>
#include <baseline/Hash.h>


using namespace baseline;

namespace medium {

class DataSplitterOutputStream : public OutputStream
{
public:

  class Callback
  {
  public:
    virtual OutputStream* createOutput() = 0;
    virtual void onNewBlock( const HashCode& code ) = 0;
  };


  DataSplitterOutputStream( Callback* );

  void close();
  int write( uint8_t* buf, size_t offset, size_t len );

private:
  Callback* mCallback;
  RabinKarpHash mRollingHash;
  baseline::CircleBuffer<uint8_t> mWindow;
  OutputStream* mOutput;
  up<HashFunction> mStrongHash;
};

} // namespace


#endif // MEDIUM_DATASPLITTER_H_
