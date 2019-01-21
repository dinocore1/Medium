#ifndef MEDIUM_DATASPLITTER_H_
#define MEDIUM_DATASPLITTER_H_

#include <baseline/Baseline.h>
#include <baseline/Streams.h>
#include <baseline/CircleBuffer.h>


using namespace baseline;

namespace medium {

class DataSplitterOutpuStream : public OutputStream
{
public:

  class Callback
  {
  public:
    virtual OutputStream* createOutput() = 0;
  };


  DataSplitterOutpuStream( Callback* );

  void close();
  int write( uint8_t* buf, size_t offset, size_t len );

private:
  Callback* mOnNewStream;
  RabinKarpHash mRollingHash;
  baseline::CircleBuffer<uint8_t> mWindow;
  OutputStream* mOutput;
};

} // namespace


#endif // MEDIUM_DATASPLITTER_H_
