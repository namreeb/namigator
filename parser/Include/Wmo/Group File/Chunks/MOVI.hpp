#pragma once

#include "Wmo/Group File/WmoGroupChunk.hpp"

#include <vector>
#include <cstdint>

namespace parser
{
namespace input
{
class MOVI : public WmoGroupChunk
{
    public:
        std::vector<std::int16_t> Indices;

        MOVI(size_t position, utility::BinaryStream *fileStream);
};
}
}