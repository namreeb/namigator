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
                std::unique_ptr<WmoChunkHeader> header(Reader->AllocateAndReadStruct<WmoChunkHeader>());

                Size = header->Size;
                Type = header->Type;
            }

            ~WmoRootChunk()
            {
                Reader = nullptr;
                Position = 0L;
                Size = 0;
                Type = 0;
            }
    };

    class WmoRootChunkType
    {
        static const unsigned int MOHD = 0x44484F4D;
        static const unsigned int MOTX = 0x58544F4D;
        static const unsigned int MOMT = 0x544D4F4D;
        static const unsigned int MOGN = 0x4E474F4D;
        static const unsigned int MOGI = 0x49474F4D;
        static const unsigned int MOSB = 0x42534F4D;
        static const unsigned int MOPV = 0x56504F4D;
        static const unsigned int MOPT = 0x54504F4D;
        static const unsigned int MOPR = 0x52504F4D;
        static const unsigned int MOVV = 0x56564F4D;
        static const unsigned int MOVB = 0x42564F4D;
        static const unsigned int MOLT = 0x544C4F4D;
        static const unsigned int MODS = 0x53444F4D;
        static const unsigned int MODN = 0x4E444F4D;
        static const unsigned int MODD = 0x44444F4D;
        static const unsigned int MFOG = 0x474F464D;
        static const unsigned int MCVP = 0x5056434D;
    };
}