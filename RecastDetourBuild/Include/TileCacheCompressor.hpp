#include "recastnavigation/DetourTileCache/Include/DetourTileCacheBuilder.h"

#include <cstring>

// NOTE: This should be replaced with an actual compression routine
struct NullCompressor : public dtTileCacheCompressor
{
    virtual int maxCompressedSize(const int bufferSize)
    {
        return bufferSize;
    }

    virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
        unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
    {
        *compressedSize = bufferSize;
        memcpy(compressed, buffer, bufferSize);
        return DT_SUCCESS;
    }

    virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
        unsigned char* buffer, const int /*maxBufferSize*/, int* bufferSize)
    {
        *bufferSize = compressedSize;
        memcpy(buffer, compressed, compressedSize);
        return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
    }
};