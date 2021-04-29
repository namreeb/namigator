#pragma once

#include "Wmo/RootFile/WmoRootChunk.hpp"

#include <cstdint>
#include <vector>

static_assert(sizeof(char) == 1, "char must be 8 bits");

namespace parser
{
namespace input
{
struct DoodadSetInfo
{
public:
    char Name[20];
    std::uint32_t FirstDoodadIndex;
    std::uint32_t DoodadCount;

private:
    std::uint32_t _unknown;
};

class MODS : WmoRootChunk
{
public:
    const unsigned int Count;
    std::vector<DoodadSetInfo> DoodadSets;

    MODS(unsigned int doodadSetsCount, size_t position,
         utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser