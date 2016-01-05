#include "Input/Wmo/WmoChunkHeader.hpp"
#include "Input/WMO/Group File/WmoGroupChunk.hpp"

namespace parser
{
namespace input
{
WmoGroupChunk::WmoGroupChunk(long position, utility::BinaryStream *groupFileStream) : Position(position)
{
    groupFileStream->SetPosition(position);

    WmoChunkHeader header;
    groupFileStream->ReadBytes(&header, sizeof(header));

    Type = static_cast<WmoGroupChunkType>(header.Type);
    Size = header.Size;
}
}
}