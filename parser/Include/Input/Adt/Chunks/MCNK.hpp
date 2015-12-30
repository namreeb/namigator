#include <vector>
#include <memory>

#include "Misc.hpp"

#include "Input/Adt/AdtChunk.hpp"
#include "Input/Adt/Chunks/Subchunks/MCVT.hpp"
#include "Input/Adt/Chunks/Subchunks/MCLQ.hpp"
#include "Input/Adt/Chunks/Subchunks/MCNR.hpp"

#include "LinearAlgebra.hpp"

namespace parser_input
{
    //const unsigned int HoleFlags[4][4] = {
    //    {0x1,    0x2,    0x4,    0x8},
    //    {0x10,   0x20,   0x40,   0x80},
    //    {0x100,  0x200,  0x400,  0x800},
    //    {0x1000, 0x2000, 0x4000, 0x8000}
    //};

    const unsigned int HoleFlags[4][4] = {
        {0x1,     0x10,     0x100,     0x1000},
        {0x2,     0x20,     0x200,     0x2000},
        {0x4,     0x40,     0x400,     0x4000},
        {0x8,     0x80,     0x800,     0x8000}
    };

    struct MCNKInfo
    {
        unsigned int Flags;
        unsigned int IndexX;
        unsigned int IndexY;
        unsigned int LayersCount;
        unsigned int DoodadReferencesCount;
        unsigned int HeightOffset;
        unsigned int NormalOffset;
        unsigned int LayerOffset;
        unsigned int ReferencesOffset;
        unsigned int AlphaOffset;
        unsigned int AlphaSize;
        unsigned int ShadowSize;
        unsigned int ShadowOffset;
        unsigned int AreaId;
        unsigned int MapObjReferencesCount;
        unsigned int Holes;
        unsigned int Unknown[4];
        unsigned int PredTex;
        unsigned int EffectDoodadCount;
        unsigned int SoundEmittersOffset;
        unsigned int SoundEmittersCount;
        unsigned int LiquidOffset;
        unsigned int LiquidSize;
        float Position[3];
        unsigned int ColorValueOffset;
        unsigned int Properties;
        unsigned int EffectId;
    };

    class MCNK : public AdtChunk
    {
        private:
            static const int VertexCount = 9 * 9 + 8 * 8;

        public:
            bool HasHoles;
            bool HasWater;

            unsigned int WaterType;
            unsigned int AreaId;

            float Height;
            float MinZ;
            float MaxZ;

            bool HoleMap[8][8];
            std::vector<Vertex> Positions;
            std::vector<Vector3> Normals;

            std::unique_ptr<MCLQ> LiquidChunk;

            MCNK(long position, utility::BinaryStream *reader);
    };
}