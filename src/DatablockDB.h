#ifndef MEDIUM_DATABLOCKDB_H_
#define MEDIUM_DATABLOCKDB_H_

class Db;

namespace medium {

class DatablockDB
{
public:
  DatablockDB( Db* );

private:
  Db* mDatabase;

};

} // namespace

#endif // MEDIUM_DATABLOCKDB_H_