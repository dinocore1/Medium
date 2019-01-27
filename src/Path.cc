

#include "Path.h"

#include <baseline/SharedBuffer.h>

using namespace baseline;

namespace medium {

static int path_check( const char* path, bool* isend )
{

  for( int i = 0; i < 256; i++ ) {
    switch( path[i] ) {
    case '/':
      *isend = false;
      return i;

    case 0:
      *isend = true;
      return i;
    }
  }

  return 0;
}

Path Path::create( const char* path )
{
  Path retval;
  bool end = false;
  int len = strlen( path ) + 1;
  int start;
  SharedBuffer* buf = SharedBuffer::alloc( len );
  retval.mBuf = buf->data();
  memcpy( retval.mBuf, path, len );

  uint8_t* pathBuf = ( uint8_t* ) buf->data();

  for( int i = 0; i < len - 1; i++ ) {
    if( path[i] == '/' || path[i] == 0 ) {
      pathBuf[i] = 0;
      retval.mStartIdx.add( i + 1 );
    }
  }

  retval.mIdx = retval.mStartIdx.size();
  return retval;
}

Path::Path()
  : mBuf( 0 ), mIdx( -1 )
{}

Path::Path( const Path& copy )
  : mBuf( copy.mBuf ), mStartIdx( copy.mStartIdx ), mIdx( copy.mIdx )
{
  SharedBuffer::bufferFromData( mBuf )->acquire();
}

Path::~Path()
{
  SharedBuffer::bufferFromData( mBuf )->release();
}

Path Path::parent()
{
  Path retval( *this );
  retval.mIdx = mIdx - 1;
  return retval;
}

const char* Path::getPart( int idx )
{
  const char* buf = ( const char* ) mBuf;
  const char* retval = &buf[mStartIdx[idx]];
  return retval;
}

const char* Path::name()
{
  return getPart( mIdx - 1 );
}


int Path::lookup_ino( ext2_filsys fs, ext2_ino_t& ino )
{
  int err;
  ext2_ino_t dir;

  dir = fs->super->s_first_ino;
  if( mIdx == 0 ) {
    ino = dir;
    return 0;
  }

  for( int i = 1; i < mIdx; i++ ) {
    const char* name = getPart( i );
    int len = strlen( name );
    err = ext2fs_lookup( fs, dir, name, len, NULL, &ino );
    if( err != 0 ) {
      return err;
    }
  }

  return 0;

}

}