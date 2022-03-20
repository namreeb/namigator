#pragma once

#include "Wmo/GroupFile/WmoGroupChunk.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MOPY : public WmoGroupChunk
{
public:
    const int TriangleCount;

    std::vector<std::uint8_t> Flags;
    std::vector<std::uint8_t> MaterialId;

    MOPY(unsigned int version, size_t position,
         utility::BinaryStream* groupFileStream);
};
} // namespace input
} // namespace parser