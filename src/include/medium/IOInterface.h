
#ifndef IOINTERFACE_H_
#define IOINTERFACE_H_

namespace medium {

  struct IOInterface {

    typedef int (*read_f)(index_t, block_t*);
    
    read_f read;
    
  };
}


#endif // IOINTERFACE_H_