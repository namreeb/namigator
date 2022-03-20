#define NOMINMAX

#include "Adt/Chunks/MCNK.hpp"

#include "Adt/Chunks/Subchunks/MCLQ.hpp"
#include "Common.hpp"
#include "utility/MathHelper.hpp"

#include <cassert>
#include <cstring>
#include <limits>

namespace parser
{
namespace input
{
MCNK::MCNK(size_t position, utility::BinaryStream* reader, bool alpha,
    int adtX, int adtY)
    : AdtChunk(position, reader), HasHoles(false), HasWater(false),
      MaxZ(std::numeric_limits<float>::lowest()),
      MinZ(std::numeric_limits<float>::max())
{
    assert(Type == AdtChunkType::MCNK);

    // it is possible for a chunk to be empty
    if (Size == 0)
        return;

    reader->rpos(position + 8);
    auto const flags = reader->Read<std::uint32_t>();

    size_t areaOffset = 0x34;
    size_t holesOffset = 0x3C;
    size_t heightOffset = 0x14;
    size_t liquidOffset = 0x60;
    math::Vector3 offset;
    if (alpha)
    {
        areaOffset += 4;
        holesOffset += 4;
        heightOffset += 4;
        liquidOffset += 4;

        auto const chunkX = reader->Read<std::uint32_t>();
        auto const chunkY = reader->Read<std::uint32_t>();

        float worldX, worldY;
        math::Convert::ADTToWorldNorthwestCorner(adtX, adtY, worldX, worldY);
        offset.X = worldX - chunkY * MeshSettings::AdtChunkSize;
        offset.Y = worldY - chunkX * MeshSettings::AdtChunkSize;
        offset.Z = 0.f;
    }
    else
    {
        // TODO: check this!
        reader->rpos(position + 8 + 0x68);
        reader->ReadBytes(&offset, sizeof(offset));
    }

    reader->rpos(position + 8 + areaOffset);
    AreaId =
        alpha ? reader->Read<std::uint16_t>() : reader->Read<std::uint32_t>();

    memset(HoleMap, 0, sizeof(bool) * 8 * 8);

    reader->rpos(position + 8 + holesOffset);
    auto const holes = reader->Read<std::uint32_t>();   // should this be std::uint16_t?

    // holes
    HasHoles = holes != 0;
    if (HasHoles)
    {
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
            {
                if (!(holes & HoleFlags[y][x]))
                    continue;

                const int curRow = 1 + (y * 2);
                const int curCol = 1 + (x * 2);

                HoleMap[curCol - 1][curRow - 1] = HoleMap[curCol][curRow - 1] =
                    HoleMap[curCol - 1][curRow] = HoleMap[curCol][curRow] =
                        true;
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

    // Information.IndexX and Information.IndexY specify the colum and row
    // (respectively) of the chunk in the tile Information.Position specifies
    // the top left corner of the current chunk (using in-game coords) these x
    // and y correspond to rows and columns (respectively) of the current chunk
    reader->rpos(position + 8 + heightOffset);
    heightOffset = reader->Read<std::uint32_t>();

    // alpha data has offset from end of header
    // TODO: do this elsewhere too
    if (alpha)
        heightOffset += 8 + 0x80;

    const MCVT heightChunk(Position + heightOffset, reader);

    constexpr float quadSize = MeshSettings::AdtSize / 128.f;

    Positions.reserve(VertexCount);
    for (int i = 0; i < VertexCount; ++i)
    {
        // if i % 17 > 8, this is the inner part of a quad
        const int x = (i % 17 > 8 ? (i - 9) % 17 : i % 17);
        const int y = i / 17;
        const float innerOffset = (i % 17 > 8 ? quadSize / 2.f : 0.f);

        const float z = offset.Z + heightChunk.Heights[i];

        Heights[i] = z;

        if (z > MaxZ)
            MaxZ = z;
        if (z < MinZ)
            MinZ = z;

        Positions.push_back({offset.X - (y * quadSize) - innerOffset,
                             offset.Y - (x * quadSize) - innerOffset, z});
    }

    reader->rpos(position + 8 + liquidOffset);
    liquidOffset = reader->Read<std::uint32_t>();
    if (alpha)
        liquidOffset += 8 + 0x80;

    if (alpha)
        HasWater = (flags & LiquidFlags::Any) != 0;
    else
    {
        auto const liquidSize = reader->Read<std::uint32_t>();
        HasWater = liquidSize > 8;
    }

    if (HasWater)
        LiquidChunk = std::make_unique<MCLQ>(Position + liquidOffset, reader,
                                             alpha, flags);
}
} // namespace input
} // namespace parser
