#pragma once

#include <vector>

#include "Input/Wmo/WmoDoodad.hpp"
#include "Input/Wmo/Root File/WmoRootChunk.hpp"

#include "LinearAlgebra.hpp"

namespace parser_input
{
    class MODD : WmoRootChunk
    {
        public:
            int Count;
            std::vector<WmoDoodadIndexedInfo> Doodads;

            MODD(long position, utility::BinaryStream *reader);
    };
}