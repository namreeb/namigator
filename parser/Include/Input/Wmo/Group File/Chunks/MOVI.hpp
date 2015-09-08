#pragma once

#include <vector>

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"

namespace parser_input
{
    class MOVI : public WmoGroupChunk
    {
        public:
            std::vector<unsigned short> Indices;

            MOVI(long position, utility::BinaryStream *fileStream);
    };
}