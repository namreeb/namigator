#pragma once

#include "utility/Include/BinaryStream.hpp"

#include <string>
#include <memory>

namespace parser
{
namespace input
{
class WowFile
{
    protected:
		std::unique_ptr<utility::BinaryStream> Reader;

        long GetAnyChunkLocation(long startLoc);
		long GetChunkLocation(const std::string &chunkName, int startLoc = 0);

    public:
        WowFile(const std::string &path);
};
}
}