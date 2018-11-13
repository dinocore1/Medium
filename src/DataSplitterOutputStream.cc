#include "stdafx.h"
#include "Rabinkarp.h"
#include "CircleBuffer.h"
#include <medium/DataSplitterOutputStream.h>

namespace medium {

#define WINDOW_SIZE 16
#define DIFFICULTY 0x00000FFF

//should be 1 split on average in 1 MiB

  DataSplitterOutpuStream::DataSplitterOutpuStream(const newstream_t& newStream)
    : mOnNewStream(newStream), mRollingHash(WINDOW_SIZE), mWindow(WINDOW_SIZE), mOutput(nullptr)
  {}

  void DataSplitterOutpuStream::close()
  {
    if (mOutput) {
      mOutput->close();
      mOutput = nullptr;
    }
  }

  void DataSplitterOutpuStream::write(byte* buf, int len)
  {
    if (mOutput == nullptr) {
      mOutput = mOnNewStream();
    }
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
      if (rollingHash <= DIFFICULTY) {
        mOutput->close();
        mOutput = mOnNewStream();
      }

      mOutput->write(&buf[i], 1);
    }
  }

}