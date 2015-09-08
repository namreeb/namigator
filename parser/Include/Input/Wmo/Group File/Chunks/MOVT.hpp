#pragma once

#include <vector>

#include "LinearAlgebra.hpp"
#include "Input/Wmo/Group File/WmoGroupChunk.hpp"
#include "BinaryStream.hpp"

using namespace utility;

namespace parser_input
{
    class MOVT : WmoGroupChunk
    {
        public:
            std::vector<Vertex> Vertices;

            MOVT(long position, utility::BinaryStream *groupFileStream);
    };
}