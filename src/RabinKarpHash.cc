
#include "stdafx.h"
#include "Rabinkarp.h"

#include <random>

namespace medium {

  static const RabinKarpHash::hashvalue_t B = 37;

  static struct Global {
    Global() {

      std::default_random_engine generator(1);
      //std::uniform_int_distribution<RabinKarpHash::hashvalue_t> distro(1, 255);

      for (int i = 0; i < 256; i++) {
        hashTable[i] = generator();
      }
    }

    RabinKarpHash::hashvalue_t hashTable[256];
  } gTable;

  

  RabinKarpHash::RabinKarpHash(uint32_t len)
    : mLen(len), mBtoN(1), mHashValue(0)
  {
    for (int i = 0; i < mLen; i++) {
      mBtoN *= B;
    }
  }

  void RabinKarpHash::reset()
  {
    mHashValue = 0;
  }

  void RabinKarpHash::eat(uint8_t in)
  {
    mHashValue = (B*mHashValue + gTable.hashTable[in]);
  }


  void RabinKarpHash::update(uint8_t in, uint8_t out)
  {
    mHashValue = (B*mHashValue + gTable.hashTable[in] - mBtoN * gTable.hashTable[out]);
  }


}