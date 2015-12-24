#pragma once

#include <vector>

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"

namespace parser_input
{
    class MOPY : public WmoGroupChunk
    {
        public:
            const int TriangleCount;

            std::vector<unsigned char> Flags;
            std::vector<unsigned char> MaterialId;

            MOPY(long position, utility::BinaryStream *groupFileStream);
    };
}