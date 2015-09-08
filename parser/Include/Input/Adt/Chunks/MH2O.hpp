#pragma once

#include <memory>
#include "BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"

namespace parser_input
{
    struct MH2OData
    {
        unsigned short Type;
        unsigned short Flags;

        float HeightLevels[2];

        unsigned char XOffset;
        unsigned char YOffset;
        unsigned char Width;
        unsigned char Height;

        int MaskOffset;
        int HeightmapOffset;
    };

    struct MH2OHeader
    {
        int DataOffset;
        int LayerCount;
        int RenderOffset;
    };

    struct MH2OBlock
    {
        std::unique_ptr<MH2OData> Data;
        std::unique_ptr<MH2OHeader> Header;

        // quad heights
        float Heights[8][8];
        unsigned char RenderMask[8];
    };

    class MH2O : AdtChunk
    {
        public:
            MH2OBlock *Blocks[16][16];

            MH2O(long position, utility::BinaryStream *reader);
            ~MH2O();
    };
}