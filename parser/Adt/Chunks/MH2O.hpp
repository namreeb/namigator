#pragma once

#include "Adt/AdtChunk.hpp"

#include "utility/BinaryStream.hpp"

#include <memory>
#include <cstdint>

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
            std::int32_t InstancesOffset;
            std::int32_t LayerCount;
            std::int32_t AttributesOffset;
        };

        struct LiquidInstance
        {
            std::uint16_t Type;
            std::uint16_t Flags;

            float MinHeight;
            float MaxHeight;

            std::uint8_t XOffset;
            std::uint8_t YOffset;
            std::uint8_t Width;
            std::uint8_t Height;

            std::uint32_t OffsetExistsBitmap;
            std::uint32_t OffsetVertexData;
        };

    public:
        std::vector<std::unique_ptr<LiquidLayer>> Layers;

        MH2O(size_t position, utility::BinaryStream *reader);
};
}
}