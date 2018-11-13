
#include "stdafx.h"
#include <medium/Medium.h>

#define BUFFER_SIZE 4096

namespace medium {

  int pump(InputStream* in, OutputStream* out)
  {
    int bytesAvailable;
    byte buffer[BUFFER_SIZE];

    while ((bytesAvailable = in->read(buffer, BUFFER_SIZE)) > 0) {
      out->write(buffer, bytesAvailable);
    }

    out->close();
    in->close();

    return 0;
  }

  void NullOutputStream::close()
  {}

  void NullOutputStream::write(byte* buf, int len)
  {}

}