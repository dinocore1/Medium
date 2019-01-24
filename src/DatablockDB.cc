#include "DatablockDB.h"

#include <db_cxx.h>

#include <baseline/Baseline.h>

namespace medium {

DatablockDB::DatablockDB( Db* db )
  : mDatabase( db )
{}

}

