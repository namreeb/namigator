#pragma once

#include "utility/Include/BinaryStream.hpp"

#include <string>

#define FILE_DUMP_BUFFER_SIZE    4096

namespace parser
{
namespace input
{
class WowFile
{
    protected:
        long GetAnyChunkLocation(long startLoc);

    public:
        utility::BinaryStream *const Reader;
        WowFile(const std::string &path);

        long GetChunkLocation(const std::string &chunkName, int startLoc = 0);
};
}
}