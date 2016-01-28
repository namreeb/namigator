#pragma once

#include "Adt/AdtChunk.hpp"
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

        MCNR(size_t position, utility::BinaryStream *reader);
};
}
}