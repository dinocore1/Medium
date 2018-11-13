#ifndef DATASPLITTER_H_
#define DATASPLITTER_H_

#include <functional>

namespace medium {

  class DataSplitterOutpuStream : public OutputStream {
  public:
    typedef std::function<sp<OutputStream>()> newstream_t;

    DataSplitterOutpuStream(const newstream_t&);

    virtual void close();
    virtual void write(byte* buf, int len);

  private:
    newstream_t mOnNewStream;
    RabinKarpHash mRollingHash;
    CircleBuffer<byte> mWindow;
    sp<OutputStream> mOutput;
  };
}


#endif // !DATASPLITTER_H_
