#pragma once

#include "Input/Wmo/WmoDoodad.hpp"
#include "Input/Wmo/Root File/WmoRootChunk.hpp"

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MODD : WmoRootChunk
{
    public:
        const int Count;
        std::vector<WmoDoodadIndexedInfo> Doodads;

        MODD(long position, utility::BinaryStream *reader);
};
}
}