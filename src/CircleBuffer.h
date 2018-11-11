#ifndef CIRCLEBUFFER_H_
#define CIRCLEBUFFER_H_

namespace medium {

  template<typename T>
  class CircleBuffer {
  public:
   CircleBuffer(int size);
   ~CircleBuffer();

   inline int size();
   void put(const T&);
   T get();

  private:
    int mMaxSize;
    int mSize;
    T* mBuffer;
    int mStart;
    int mEnd;

  };

  /////////////// Implementation ////////////////////

  template<typename T>
  CircleBuffer<T>::CircleBuffer(int size)
    : mMaxSize(size), mSize(0), mStart(0), mEnd(0)
  {
    mBuffer = new T[mMaxSize];
  }

  template<typename T>
  CircleBuffer<T>::~CircleBuffer()
  {
    delete[] mBuffer;
  }

  template<typename T>
  int CircleBuffer<T>::size()
  {
    return mSize;
  }

  template<typename T>
  void CircleBuffer<T>::put(const T& v)
  {
    if (mSize < mMaxSize) {
      mEnd = (mEnd + 1) % mMaxSize;
      mBuffer[mEnd] = v;
      mSize++;
    }
  }

  template<typename T>
  T CircleBuffer<T>::get()
  {
    T retval;
    if (mSize > 0) {
      retval = mBuffer[mStart];
      mStart = (mStart + 1) % mMaxSize;
      mSize--;
    }

    return retval;
  }

}

#endif // CIRCLEBUFFER_H_