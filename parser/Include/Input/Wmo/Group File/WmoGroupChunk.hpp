#pragma once

#include "Input/Wmo/WmoChunkHeader.hpp"
#include "BinaryStream.hpp"

using namespace utility;

namespace parser_input
{
    class WmoGroupChunk
    {
        public:
            const long Position;

            unsigned int Size;
            unsigned int Type;

            WmoGroupChunk(long position, utility::BinaryStream *groupFileStream);
    };

    class WmoGroupChunkType
    {
        public:
            static const unsigned int MOGP = 0x50474F4D;
            static const unsigned int MOPY = 0x59504F4D;
            static const unsigned int MOVI = 0x49564F4D;
            static const unsigned int MOVT = 0x54564F4D;
            static const unsigned int MONR = 0x524E4F4D;
            static const unsigned int MOTV = 0x56544F4D;
            static const unsigned int MOBA = 0x41424F4D;
            static const unsigned int MOLR = 0x524C4F4D;
            static const unsigned int MODR = 0x52444F4D;
            static const unsigned int MOBN = 0x4E424F4D;
            static const unsigned int MOBR = 0x52424F4D;
            static const unsigned int MOCV = 0x56434F4D;
            static const unsigned int MLIQ = 0x51494C4D;
    };
}