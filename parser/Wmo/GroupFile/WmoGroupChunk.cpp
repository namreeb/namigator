#include "Wmo/GroupFile/WmoGroupChunk.hpp"

#include "Wmo/WmoChunkHeader.hpp"

namespace parser
{
namespace input
{
WmoGroupChunk::WmoGroupChunk(size_t position,
                             utility::BinaryStream* groupFileStream)
    : Position(position)
{
    groupFileStream->rpos(position);

    WmoChunkHeader header;
    groupFileStream->ReadBytes(&header, sizeof(header));

    Type = static_cast<WmoGroupChunkType>(header.Type);
    Size = header.Size;
}
} // namespace input
} // namespace parser