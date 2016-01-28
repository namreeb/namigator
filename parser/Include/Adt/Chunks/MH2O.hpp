#pragma once

#include "Adt/AdtChunk.hpp"

#include "utility/Include/BinaryStream.hpp"

#include <memory>

namespace parser
{
namespace input
{
struct LiquidLayer
{
    int X;
    int Y;

    bool Render[8][8];
    float Heights[9][9];
};

class MH2O : AdtChunk
{
    private:
        struct MH2OHeader
        {
            int InstancesOffset;
            int LayerCount;
            int AttributesOffset;
        };

        struct LiquidInstance
        {
            unsigned short Type;
            unsigned short Flags;

            float MinHeight;
            float MaxHeight;

            unsigned __int8 XOffset;
            unsigned __int8 YOffset;
            unsigned __int8 Width;
            unsigned __int8 Height;

            unsigned int OffsetExistsBitmap;
            unsigned int OffsetVertexData;
        };

    public:
        std::vector<std::unique_ptr<LiquidLayer>> Layers;

        MH2O(size_t position, utility::BinaryStream *reader);
};
}
}