#ifndef MEDIUM_INODEDB_H_
#define MEDIUM_INODEDB_H_

#include <db_cxx.h>

#include <baseline/Baseline.h>
#include <baseline/Mutex.h>

class Db;

#define MAX_FILENAME_LEN 64

namespace medium {

class InodeDB
{
public:

  struct INode {
    char name[MAX_FILENAME_LEN];
    uint64_t fileSizeFlags;
    DB_HEAP_RID ptrs[16];

    uint64_t getFileSize();
    uint8_t getFlags();

  };

  InodeDB( Db* );
  int init();

  int getAttr( const char* path, struct stat* st );


private:
  Db* mDatabase;
  baseline::Mutex mLock;
};

} // namespace

#endif // MEDIUM_INODEDB_H_