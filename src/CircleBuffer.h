#ifndef CIRCLEBUFFER_H_
#define CIRCLEBUFFER_H_

namespace medium {

  template<typename T>
  class CircleBuffer {
  public:
   CircleBuffer(int size);
   ~CircleBuffer();

   inline int size() const;
   void put(const T&);
   T get();

  private:
    int mMaxSize;
    int mSize;
    T* mBuffer;
    int mHead;
    int mTail;

  };

  /////////////// Implementation ////////////////////

  template<typename T>
  CircleBuffer<T>::CircleBuffer(int size)
    : mMaxSize(size), mSize(0), mHead(0), mTail(0)
  {
    mBuffer = new T[mMaxSize];
  }

  template<typename T>
  CircleBuffer<T>::~CircleBuffer()
  {
    delete[] mBuffer;
  }

  template<typename T>
  int CircleBuffer<T>::size() const
  {
    return mSize;
  }

  template<typename T>
  void CircleBuffer<T>::put(const T& v)
  {
    if (mSize < mMaxSize) {
      mBuffer[mTail] = v;
      mTail = (mTail + 1) % mMaxSize;
      mSize++;
    }
  }

  template<typename T>
  T CircleBuffer<T>::get()
  {
    T retval;
    if (mSize > 0) {
      retval = mBuffer[mHead];
      mHead = (mHead + 1) % mMaxSize;
      mSize--;
    }

    return retval;
  }

}

#endif // CIRCLEBUFFER_H_