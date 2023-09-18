#pragma once

#include "Wmo/GroupFile/WmoGroupChunk.hpp"
#include "utility/Array2d.hpp"

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

    float Corner[3];
    std::unique_ptr<utility::Array2d<float>> Heights;

    MLIQ(size_t position, utility::BinaryStream* groupFileStream,
         unsigned int version);
};
} // namespace input
} // namespace parser