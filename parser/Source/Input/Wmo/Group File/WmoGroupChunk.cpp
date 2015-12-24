#include <memory>

#include "Input/WMO/Group File/WmoGroupChunk.hpp"

namespace parser_input
{
    WmoGroupChunk::WmoGroupChunk(long position, utility::BinaryStream *groupFileStream) : Position(position)
    {
        groupFileStream->SetPosition(position);

        WmoChunkHeader header;
        groupFileStream->ReadStruct(&header);

        Type = header.Type;
        Size = header.Size;
    }
}