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
        const long Position;

        unsigned int Size;
        WmoGroupChunkType Type;

        WmoGroupChunk(long position, utility::BinaryStream *groupFileStream);
};
}
}