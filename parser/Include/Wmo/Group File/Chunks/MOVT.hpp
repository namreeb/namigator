#pragma once

#include "utility/Include/LinearAlgebra.hpp"
#include "Wmo/Group File/WmoGroupChunk.hpp"
#include "utility/Include/BinaryStream.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MOVT : WmoGroupChunk
{
    public:
        std::vector<utility::Vertex> Vertices;

        MOVT(size_t position, utility::BinaryStream *groupFileStream);
};
}
}