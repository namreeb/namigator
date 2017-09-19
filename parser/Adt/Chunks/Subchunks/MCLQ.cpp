#include "utility/Exception.hpp"
#include "Adt/Chunks/Subchunks/MCLQ.hpp"

namespace parser
{
namespace input
{
MCLQ::MCLQ(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCLQ)
        THROW("Expected (but did not find) MCVT type");
#endif

    Altitude = reader->Read<float>();
    BaseHeight = reader->Read<float>();

    for (int y = 0; y < 9; ++y)
        for (int x = 0; x < 9; ++x)
        {
            reader->rpos(reader->rpos() + 4);
            Heights[y][x] = reader->Read<float>();
        }

    reader->ReadBytes(&RenderMap, sizeof(RenderMap));
}
}
}