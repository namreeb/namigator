#pragma once

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"
#include "utility/Include/Array2d.hpp"

#include <memory>

namespace parser
{
namespace input
{
class MLIQ : WmoGroupChunk
{
    public:
        unsigned int Width;
        unsigned int Height;

        float Base[3];
        std::unique_ptr<utility::Array2d<float>> Heights;

        MLIQ(long position, utility::BinaryStream *groupFileStream);
};
}
}