#pragma once

#include "Input/Adt/AdtChunk.hpp"
#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
class MCNR : AdtChunk
{
    public:
        struct
        {
            int8_t Normal[3];
        } Entries[9 * 9 + 8 * 8];

        MCNR(long position, utility::BinaryStream *reader);
};
}
}