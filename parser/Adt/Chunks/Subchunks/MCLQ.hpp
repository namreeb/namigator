#pragma once

#include "Adt/AdtChunk.hpp"
#include "utility/BinaryStream.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
enum LiquidFlags
{
    Water = 0x4,
    Ocean = 0x8,
    Magma = 0x10,
    Slime = 0x20,
    Any = (Water | Ocean | Magma | Slime)
};

class MCLQ : AdtChunk
{
public:
    float Heights[9][9];
    std::uint8_t RenderMap[8][8];

    float Altitude;
    float BaseHeight;

    MCLQ(size_t position, ::utility::BinaryStream* reader, bool alpha,
         unsigned int flags);
};
} // namespace input
} // namespace parser