#pragma once

#include "Adt/AdtChunk.hpp"

#include "utility/Include/BinaryStream.hpp"

#include <Windows.h>

namespace parser
{
namespace input
{
class MCLQ : AdtChunk
{
    public:
        float Heights[9][9];
        BYTE RenderMap[8][8];

        float Altitude;
        float BaseHeight;

        MCLQ(size_t position, ::utility::BinaryStream *reader);
};
}
}