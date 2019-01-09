#pragma once

#include "Wmo/GroupFile/WmoGroupChunk.hpp"

#include "utility/Vector.hpp"
#include "utility/BinaryStream.hpp"

#include <vector>

namespace parser
{
namespace input
{
class MOVT : WmoGroupChunk
{
    public:
        std::vector<math::Vertex> Vertices;

        MOVT(size_t position, utility::BinaryStream *groupFileStream);
};
}
}