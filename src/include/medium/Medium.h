
#ifndef MEDIUM_H_
#define MEDIUM_H_

#include <memory>

namespace medium {

  typedef uint8_t byte;

  template<typename T>
  using sp = std::shared_ptr<T>;

  class OutputStream {
  public:
    virtual void close() = 0;
    virtual void write(byte* buf, int len) = 0;
  };

  class InputStream {
  public:
    virtual void close() = 0;

    /**
    * read up to maxLen bytes into buffer.
    * @param buf
    * @param maxLen maximum number of bytes to read into buf
    * @return number of bytes read, or -1 if end-of-file
    */
    virtual int read(byte* buf, int maxLen) = 0;
  };

  int pump(InputStream*, OutputStream*);

  class NullOutputStream : public OutputStream {
  public:
    void close();
    void write(byte*, int len);
  };
}

#endif // MEDIUM_H_