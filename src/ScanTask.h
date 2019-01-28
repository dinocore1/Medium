#ifndef MEDIUM_SCANTASK_H_
#define MEDIUM_SCANTASK_H_

#include <ext2fs/ext2fs.h>

#include "ext2fs/fuse_ext2.h"

#include <baseline/Baseline.h>
#include <baseline/ExecutorService.h>

#include "RabinKarp.h"
#include "DataSplitterOutputStream.h"


namespace medium {

class ScanTask : public baseline::Runnable, public DataSplitterOutputStream::Callback
{
public:
  ScanTask( Ext2FS& livefs, ext2_ino_t, Ext2FS& partsFS );
  void run();

  baseline::OutputStream* createOutput();
  void onNewBlock( const baseline::HashCode& );

  Ext2FS& mLiveFS;
  ext2_ino_t mIno;
  Ext2FS& mPartsFS;
};

} // medium




#endif // MEDIUM_SCANTASK_H_