#pragma once

#include "Adt/AdtChunk.hpp"
#include "Doodad/DoodadPlacement.hpp"

#include "utility/Include/BinaryStream.hpp"

#include <vector>
#include <memory>

namespace parser
{
namespace input
{
class MDDF : AdtChunk
{
    public:
        std::vector<DoodadPlacement> Doodads;

        MDDF(size_t position, utility::BinaryStream *reader);
};
}
}