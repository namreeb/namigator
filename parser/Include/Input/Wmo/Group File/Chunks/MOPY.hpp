#pragma once

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"

namespace parser_input
{
    class MOPY : public WmoGroupChunk
    {
        public:
            int TriangleCount;
            unsigned char *Flags;
            unsigned char *MaterialId;

            MOPY(long position, utility::BinaryStream *groupFileStream);
            ~MOPY();
    };
}