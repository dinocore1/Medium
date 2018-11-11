#ifndef DATASPLITTER_H_
#define DATASPLITTER_H_

namespace medium {

  class DataSplitter : public OutputStream {
  public:
    DataSplitter();

    virtual void close();
    virtual void write(byte* buf, int len);

  private:
    RabinKarpHash mRollingHash;
    CircleBuffer<byte> mWindow;
  };
}


#endif // !DATASPLITTER_H_
