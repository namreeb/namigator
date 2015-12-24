#pragma once

#include <utility>

#include "Input/Wmo/Group File/WmoGroupChunk.hpp"
#include "Array2d.hpp"

namespace parser_input
{
    class MLIQ : WmoGroupChunk
    {
        public:
            unsigned int Width;
            unsigned int Height;

            float Base[3];
            std::unique_ptr<Array2d<float>> Heights;

            MLIQ(long position, utility::BinaryStream *groupFileStream);
    };
}