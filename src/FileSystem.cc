#include <baseline/Baseline.h>

#include "Filesystem.h"

namespace medium {

#define FILEBLOCK_TAG "FileBlockStorage"

struct FileBlockStorageHeader {
    char magic[8];
    uint8_t reserved[8];
    uint32_t numBlocks;
};

FileBlockStorage::FileBlockStorage(FILE* file)
 : mFD(file)
{}

void FileBlockStorage::close()
{
    fclose(mFD);
    mFD = nullptr;
}

int FileBlockStorage::readBlock(uint32_t id, uint8_t* buf, size_t offset)
{
    size_t err;
    long f_offset = sizeof(FileBlockStorageHeader) + (id * BLOCK_SIZE);
    fseek(mFD, f_offset, SEEK_SET);
    if((err = fread(buf, sizeof(uint8_t), BLOCK_SIZE, mFD)) != BLOCK_SIZE){
        LOG_ERROR(FILEBLOCK_TAG, "error reading block: %d", id);
        return -1;
    }
    return 0;
}

int FileBlockStorage::writeBlock(uint32_t id, uint8_t* buf, size_t offset)
{
    size_t err;
    long f_offset = sizeof(FileBlockStorageHeader) + (id * BLOCK_SIZE);
    fseek(mFD, f_offset, SEEK_SET);
    if((err = fwrite(buf, sizeof(uint8_t), BLOCK_SIZE, mFD)) != BLOCK_SIZE) {
        LOG_ERROR(FILEBLOCK_TAG, "error writing block: %d", id);
        return -1;
    }
    return 0;
}

} // namespace