#pragma once

#include "Input/Wmo/WmoChunkHeader.hpp"
#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
enum class WmoRootChunkType : unsigned int
{
    MOHD = 'DHOM',
    MOTX = 'XTOM',
    MOMT = 'TMOM',
    MOGN = 'NGOM',
    MOGI = 'IGOM',
    MOSB = 'BSOM',
    MOPV = 'VPOM',
    MOPT = 'TPOM',
    MOPR = 'RPOM',
    MOVV = 'VVOM',
    MOVB = 'BVOM',
    MOLT = 'TLOM',
    MODS = 'SDOM',
    MODN = 'NDOM',
    MODD = 'DDOM',
    MFOG = 'GOFM',
    MCVP = 'PVCM',
};

class WmoRootChunk
{
    public:
        long Position;
        unsigned int Size;
        unsigned int Type;

        utility::BinaryStream *Reader;

        WmoRootChunk(long position, utility::BinaryStream *reader)
        {
            Position = position;
            Reader = reader;

            Reader->SetPosition(position);

            WmoChunkHeader header;
            reader->ReadBytes(&header, sizeof(header));

            Size = header.Size;
            Type = header.Type;
        }
};
}
}