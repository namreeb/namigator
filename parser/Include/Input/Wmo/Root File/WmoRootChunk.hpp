#pragma once

#include "Input/Wmo/WmoChunkHeader.hpp"
#include "BinaryStream.hpp"

using namespace utility;

namespace parser_input
{
    class WmoRootChunk
    {
        public:
            long Position;
            unsigned int Size;
            unsigned int Type;

            BinaryStream *Reader;

            WmoRootChunk(long position, utility::BinaryStream *reader)
            {
                Position = position;
                Reader = reader;

                Reader->SetPosition(position);

                WmoChunkHeader header;
                reader->ReadStruct(&header);

                Size = header.Size;
                Type = header.Type;
            }
    };

    class WmoRootChunkType
    {
        static const unsigned int MOHD = 'DHOM';
        static const unsigned int MOTX = 'XTOM';
        static const unsigned int MOMT = 'TMOM';
        static const unsigned int MOGN = 'NGOM';
        static const unsigned int MOGI = 'IGOM';
        static const unsigned int MOSB = 'BSOM';
        static const unsigned int MOPV = 'VPOM';
        static const unsigned int MOPT = 'TPOM';
        static const unsigned int MOPR = 'RPOM';
        static const unsigned int MOVV = 'VVOM';
        static const unsigned int MOVB = 'BVOM';
        static const unsigned int MOLT = 'TLOM';
        static const unsigned int MODS = 'SDOM';
        static const unsigned int MODN = 'NDOM';
        static const unsigned int MODD = 'DDOM';
        static const unsigned int MFOG = 'GOFM';
        static const unsigned int MCVP = 'PVCM';
    };
}