#include "ext2fs/fuse_ext2.h"

namespace medium {

int Ext2FS::do_check( const char* path )
{
  const char* basename_path;
  basename_path = strrchr( path, '/' );
  if( basename_path == NULL ) {
    LOG_ERROR( LOG_TAG, "this should not happen %s", path );
    return -ENOENT;
  }
  basename_path++;
  if( strlen( basename_path ) > 255 ) {
    LOG_ERROR( LOG_TAG, "basename exceeds 255 characters %s", path );
    return -ENAMETOOLONG;
  }
  return 0;
}

int Ext2FS::do_check_split( const char* path, char** dirname, char** basename )
{
  char* tmp;
  char* cpath = strdup( path );
  tmp = strrchr( cpath, '/' );
  if( tmp == NULL ) {
    //LOG_ERROR( LOG_TAG, "this should not happen %s", path);
    free( cpath );
    return -ENOENT;
  }
  *tmp = '\0';
  tmp++;
  if( strlen( tmp ) > 255 ) {
    //LOG_ERROR( LOG_TAG, "basename exceeds 255 characters %s",path);
    free( cpath );
    return -ENAMETOOLONG;
  }
  *dirname = cpath;
  *basename = tmp;
  return 0;
}

void Ext2FS::free_split( char* dirname, char* basename )
{
  free( dirname );
}

}