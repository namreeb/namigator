#include "Adt/Chunks/Subchunks/MCLQ.hpp"

#include <cassert>

namespace parser
{
namespace input
{
MCLQ::MCLQ(size_t position, utility::BinaryStream* reader, bool alpha,
    unsigned int flags)
    : AdtChunk(position, reader)
{
    if (alpha)
        reader->rpos(position);
    else
        assert(Type == AdtChunkType::MCLQ);

    Altitude = reader->Read<float>();
    BaseHeight = reader->Read<float>();

    for (int y = 0; y < 9; ++y)
        for (int x = 0; x < 9; ++x)
        {
            reader->rpos(reader->rpos() + 4);
            if (flags & (LiquidFlags::Water | LiquidFlags::Magma))
                Heights[y][x] = reader->Read<float>();
            else if (flags & LiquidFlags::Ocean)
                Heights[y][x] = Altitude;
        }

    reader->ReadBytes(&RenderMap, sizeof(RenderMap));
}
} // namespace input
} // namespace parser