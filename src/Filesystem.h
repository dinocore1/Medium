#ifndef MEDIUM_FILESYSTE_H_
#define MEDIUM_FILESYSTE_H_

#include <cstdio>

namespace medium {

#define MAX_FILENAME_LEN 63
#define BLOCK_SIZE 4096

class BlockStorage {
public:
    
    virtual void close() = 0;
    virtual int readBlock(uint32_t id, uint8_t* buf, size_t offset) = 0;
    virtual int writeBlock(uint32_t id, uint8_t* buf, size_t offset) = 0;

};

struct INode {
    char name[MAX_FILENAME_LEN];
    uint8_t flags;
    uint32_t blockPtrs[16];
};

class FileBlockStorage : public BlockStorage {
public:

    static int open(const char* path, FileBlockStorage&);

    int format(uint32_t numBlocks);

    void close() override;
    int readBlock(uint32_t id, uint8_t* buf, size_t offset) override;
    int writeBlock(uint32_t id, uint8_t* buf, size_t offset) override;

private:
    FILE* mFD;
    uint32_t mNumBlocks;
};


} // namespace

#endif // MEDIUM_FILESYSTE_H_