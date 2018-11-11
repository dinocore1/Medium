
#include <medium/Medium.h>
#include <medium/IOInterface.h>

using namespace medium;

static
int my_read(index_t block, block_t* dest)
{

}

int main() {

  IOInterface iface = {
    my_read,
  };


}