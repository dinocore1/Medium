#include "stdafx.h"
#include "Rabinkarp.h"
#include "CircleBuffer.h"
#include <medium/DataSplitter.h>

namespace medium {

#define WINDOW_SIZE 16

  DataSplitter::DataSplitter()
    : mRollingHash(WINDOW_SIZE), mWindow(WINDOW_SIZE)
  {}

  void DataSplitter::close()
  {}

  void DataSplitter::write(byte* buf, int len)
  {
    for (int i = 0; i < len; i++) {
      byte in = buf[i];
      if (mWindow.size() < WINDOW_SIZE) {
        mRollingHash.eat(in);
      }
      else {
        byte out = mWindow.get();
        mRollingHash.update(in, out);
      }
      mWindow.put(in);

      RabinKarpHash::hashvalue_t rollingHash = mRollingHash.hash();

    }

    
  }

}