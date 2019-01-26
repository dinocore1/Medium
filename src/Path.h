#ifndef MEDIUM_PATH_H_
#define MEDIUM_PATH_H_

#include <ext2fs/ext2fs.h>

#include <baseline/Baseline.h>
#include <baseline/String8.h>
#include <baseline/Vector.h>

namespace medium {

class Path {
public:
    static Path create(const char*);
    Path();
    Path(const Path&);
    ~Path();

    Path parent();
    const char* name();
    const char* getPart(int idx);

    int lookup_ino(ext2_filsys fs, ext2_ino_t& ino);

private:
    void* mBuf;
    baseline::Vector<int> mStartIdx;
    int mIdx;

};

} // namespace

#endif // MEDIUM_PATH_H_