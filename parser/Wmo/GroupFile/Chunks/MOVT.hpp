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
        std::vector<math::Vector3> Vertices;

        MOVT(size_t position, utility::BinaryStream *groupFileStream);
};
}
}