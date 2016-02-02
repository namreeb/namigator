#include "Adt/AdtChunk.hpp"
#include "Adt/Chunks/Subchunks/MCVT.hpp"
#include "Adt/Chunks/Subchunks/MCLQ.hpp"

#include "utility/Include/Exception.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <memory>
#include <cstdint>

namespace parser
{
namespace input
{
const unsigned int HoleFlags[4][4] =
{
    {0x1,     0x10,     0x100,     0x1000},
    {0x2,     0x20,     0x200,     0x2000},
    {0x4,     0x40,     0x400,     0x4000},
    {0x8,     0x80,     0x800,     0x8000}
};

struct MCNKInfo
{
    std::uint32_t Flags;
    std::uint32_t IndexX;
    std::uint32_t IndexY;
    std::uint32_t LayersCount;
    std::uint32_t DoodadReferencesCount;
    std::uint32_t HeightOffset;
    std::uint32_t NormalOffset;
    std::uint32_t LayerOffset;
    std::uint32_t ReferencesOffset;
    std::uint32_t AlphaOffset;
    std::uint32_t AlphaSize;
    std::uint32_t ShadowSize;
    std::uint32_t ShadowOffset;
    std::uint32_t AreaId;
    std::uint32_t MapObjReferencesCount;
    std::uint32_t Holes;
    std::uint32_t Unknown[4];
    std::uint32_t PredTex;
    std::uint32_t EffectDoodadCount;
    std::uint32_t SoundEmittersOffset;
    std::uint32_t SoundEmittersCount;
    std::uint32_t LiquidOffset;
    std::uint32_t LiquidSize;
    float Position[3];
    std::uint32_t ColorValueOffset;
    std::uint32_t Properties;
    std::uint32_t EffectId;
};

class MCNK : public AdtChunk
{
    private:
        static constexpr int VertexCount = 9 * 9 + 8 * 8;

    public:
        bool HasHoles;
        bool HasWater;

        float Height;
        float MinZ;
        float MaxZ;

        bool HoleMap[8][8];
        std::vector<utility::Vertex> Positions;

        std::unique_ptr<MCLQ> LiquidChunk;

        MCNK(size_t position, utility::BinaryStream *reader);
};
}
}