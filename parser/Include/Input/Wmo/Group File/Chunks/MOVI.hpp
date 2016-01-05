#pragma once

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MOVI : public WmoGroupChunk
{
    public:
        std::vector<unsigned short> Indices;

        MOVI(long position, utility::BinaryStream *fileStream);
};
}
}