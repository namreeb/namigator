#pragma once

#include "Wmo/GroupFile/WmoGroupChunk.hpp"

#include <vector>
#include <cstdint>

namespace parser
{
namespace input
{
class MOVI : public WmoGroupChunk
{
    public:
        std::vector<std::uint16_t> Indices;

        MOVI(size_t position, utility::BinaryStream *fileStream);
};
}
}