#pragma once

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
            Array2d<float> *Heights;
            Array2d<unsigned char> *RenderMap;

            MLIQ(long position, utility::BinaryStream *groupFileStream);
            ~MLIQ();
    };
}