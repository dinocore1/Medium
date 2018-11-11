
#ifndef RABINKARP_H_
#define RABINKARP_H_

namespace medium {

  class RabinKarpHash {
  public:
    typedef uint32_t hashvalue_t;

    RabinKarpHash(uint32_t len);

    void reset();
    void eat(uint8_t in);
    void update(uint8_t in, uint8_t out);
    inline hashvalue_t hash();

  private:
    const int mLen;
    
    hashvalue_t mHashValue;
    hashvalue_t mBtoN;
    static const hashvalue_t B = 37;
  };

  ////////////////// Implementation ///////////////////////////

  template<typename HashValue>
  RabinKarpHash(int len, int numBits)
   : mLen(len)
  {
    mBtoN = 1;
    for (int i = 0; i < mLen; i++) {
      mBtoN *= B;
      mBtoN &= mMask;
    }
  }

  template<typename HashValue>
  void RabinKarpHash<HashValue>::reset()
  {
    mHashValue = 0;
  }
   
  template<typename HashValue>
  void RabinKarpHash<HashValue>::eat(uint8_t in)
  {
    hashvalue = (B*hashvalue + hasher.hashvalues[in]) & mMask;
  }

  template<typename HashValue>
  void RabinKarpHash<HashValue>::update(uint8_t in, uint8_t out)
  {
    mHashValue = (B*mHashValue + hasher.hashvalues[in] - BtoN * hasher.hashvalues[out]) & mMask;
  }

  template<typename HashValue>
  void RabinKarpHash<HashValue>::hash()
  {
    return mHashValue;
  }

}

#endif // RABINKARP_H_