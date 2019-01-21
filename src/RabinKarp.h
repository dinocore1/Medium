
#ifndef RABINKARP_H_
#define RABINKARP_H_

namespace medium {

class RabinKarpHash
{
public:
  typedef uint32_t hashvalue_t;

  RabinKarpHash( uint32_t len );

  void reset();
  void eat( uint8_t in );
  void update( uint8_t in, uint8_t out );
  inline hashvalue_t hash();

private:
  int mLen;
  hashvalue_t mBtoN;
  hashvalue_t mHashValue;

};

////////////////// Implementation ///////////////////////////

inline
RabinKarpHash::hashvalue_t RabinKarpHash::hash()
{
  return mHashValue;
}

}

#endif // RABINKARP_H_