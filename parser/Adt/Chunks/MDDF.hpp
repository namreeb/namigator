#pragma once

#include "Adt/AdtChunk.hpp"
#include "Doodad/DoodadPlacement.hpp"
#include "utility/BinaryStream.hpp"

#include <memory>
#include <vector>

namespace parser
{
namespace input
{
class MDDF : AdtChunk
{
public:
    std::vector<DoodadPlacement> Doodads;

    MDDF(size_t position, utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser