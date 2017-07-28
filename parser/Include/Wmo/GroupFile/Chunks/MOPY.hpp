#pragma once

#include <vector>

#include "Wmo/GroupFile/WmoGroupChunk.hpp"

namespace parser
{
namespace input
{
class MOPY : public WmoGroupChunk
{
    public:
        const int TriangleCount;

        std::vector<unsigned char> Flags;
        std::vector<unsigned char> MaterialId;

        MOPY(size_t position, utility::BinaryStream *groupFileStream);
};
}
}