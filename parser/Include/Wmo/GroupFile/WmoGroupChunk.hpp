#pragma once

#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
enum class WmoGroupChunkType : unsigned int
{
    MOGP = 'PGOM',
    MOPY = 'YPOM',
    MOVI = 'IVOM',
    MOVT = 'TVOM',
    MONR = 'RNOM',
    MOTV = 'VTOM',
    MOBA = 'ABOM',
    MOLR = 'RLOM',
    MODR = 'RDOM',
    MOBN = 'NBOM',
    MOBR = 'RBOM',
    MOCV = 'VCOM',
    MLIQ = 'QILM',
};

class WmoGroupChunk
{
    public:
        const size_t Position;

        unsigned int Size;
        WmoGroupChunkType Type;

        WmoGroupChunk(size_t position, utility::BinaryStream *groupFileStream);
};
}
}