#pragma once

#include "Input/Adt/AdtChunk.hpp"
#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
class MCLQ: AdtChunk
{
    public:
        float Heights[9][9];
        BYTE RenderMap[8][8];

        float Altitude;
        float BaseHeight;

        MCLQ(long position, ::utility::BinaryStream *reader);
};
}
}