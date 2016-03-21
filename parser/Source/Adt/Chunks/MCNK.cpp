#define NOMINMAX

#include "ADT/Chunks/MCNK.hpp"
#include "ADT/Chunks/Subchunks/MCLQ.hpp"

#include "RecastDetourBuild/Include/Common.hpp"

#include <limits>

namespace parser
{
namespace input
{
MCNK::MCNK(size_t position, utility::BinaryStream *reader)
    : AdtChunk(position, reader), Height(0.f), HasHoles(false), HasWater(false),
        MaxZ(std::numeric_limits<float>::lowest()), MinZ(std::numeric_limits<float>::max())
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCNK)
        THROW("Expected (but did not find) MCNK type");
#endif

    // it is possible for a chunk to be empty
    if (Size == 0)
        return;

    MCNKInfo information;
    reader->ReadBytes(&information, sizeof(information));

    Height = information.Position[2];

    memset(HoleMap, 0, sizeof(bool)*8*8);

    // holes
    HasHoles = !!information.Holes;
    if (HasHoles)
    {
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
            {
                if (!(information.Holes & HoleFlags[y][x]))
                    continue;

                const int curRow = 1 + (y * 2);
                const int curCol = 1 + (x * 2);

                HoleMap[curCol - 1][curRow - 1] =
                    HoleMap[curCol][curRow - 1] =
                    HoleMap[curCol - 1][curRow] =
                    HoleMap[curCol][curRow]     = true;
            }
    }

    /*
        *        +X
        *     +Y    -Y
        *        -X
        *  When you go up, your ingame X goes up, but the ADT file Y goes down.
        *  When you go left, your ingame Y goes up, but the ADT file X goes down.
        *  When you go down, your ingame X goes down, but the ADT file Y goes up.
        *  When you go right, your ingame Y goes down, but the ADT file X goes up.
        */
    constexpr float quadSize = MeshSettings::AdtSize / 128.f;

    // Information.IndexX and Information.IndexY specify the colum and row (respectively) of the chunk in the tile
    // Information.Position specifies the top left corner of the current chunk (using in-game coords)
    // these x and y correspond to rows and columns (respectively) of the current chunk

    MCVT heightChunk(Position + information.HeightOffset, reader);

    Positions.reserve(VertexCount);

    for (int i = 0; i < VertexCount; ++i)
    {
        // if i % 17 > 8, this is the inner part of a quad
        const int x = (i % 17 > 8 ? (i - 9) % 17 : i % 17);
        const int y = i / 17;
        const float innerOffset = (i % 17 > 8 ? quadSize / 2.f : 0.f);

        const float z = information.Position[2] + heightChunk.Heights[i];

        if (z > MaxZ)
            MaxZ = z;
        if (z < MinZ)
            MinZ = z;

        Positions.push_back({ information.Position[0] - (y*quadSize) - innerOffset,
                              information.Position[1] - (x*quadSize) - innerOffset,
                              z });
    }

    HasWater = information.LiquidSize > 8;
    if (HasWater)
        LiquidChunk.reset(new MCLQ(Position + information.LiquidOffset, reader));
}
}
}