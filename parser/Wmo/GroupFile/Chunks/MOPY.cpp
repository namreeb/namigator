#include "Wmo/GroupFile/Chunks/MOPY.hpp"

namespace parser
{
namespace input
{
MOPY::MOPY(unsigned int version, size_t position,
    utility::BinaryStream* groupFileStream)
    : WmoGroupChunk(position, groupFileStream),
    TriangleCount(Size / (version == 14 ? 4 : 2))
{
    groupFileStream->rpos(position + 8);

    Flags.reserve(TriangleCount);
    MaterialId.reserve(TriangleCount);

    for (int i = 0; i < TriangleCount; ++i)
    {
        Flags.push_back(groupFileStream->Read<std::uint8_t>());
        if (version == 14)
            groupFileStream->rpos(groupFileStream->rpos() + 1);
        MaterialId.push_back(groupFileStream->Read<std::uint8_t>());
        if (version == 14)
            groupFileStream->rpos(groupFileStream->rpos() + 1);
    }
}
} // namespace input
} // namespace parser