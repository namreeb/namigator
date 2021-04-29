#pragma once

#include "Wmo/RootFile/WmoRootChunk.hpp"
#include "Wmo/WmoDoodadPlacement.hpp"
#include "utility/BinaryStream.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MODD : WmoRootChunk
{
public:
    const int Count;
    std::vector<WmoDoodadPlacement> Doodads;

    MODD(size_t position, utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser