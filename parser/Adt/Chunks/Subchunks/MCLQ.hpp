#pragma once

#include "Adt/AdtChunk.hpp"

#include "utility/BinaryStream.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
class MCLQ : AdtChunk
{
    public:
        float Heights[9][9];
        std::uint8_t RenderMap[8][8];

        float Altitude;
        float BaseHeight;

        MCLQ(size_t position, ::utility::BinaryStream *reader);
};
}
}