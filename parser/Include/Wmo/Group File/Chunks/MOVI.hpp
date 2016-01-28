#pragma once

#include "Wmo/Group File/WmoGroupChunk.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MOVI : public WmoGroupChunk
{
    public:
        std::vector<unsigned short> Indices;

        MOVI(size_t position, utility::BinaryStream *fileStream);
};
}
}