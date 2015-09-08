#include <memory>

#include "Input/WMO/Group File/WmoGroupChunk.hpp"

namespace parser_input
{
    WmoGroupChunk::WmoGroupChunk(long position, utility::BinaryStream *groupFileStream)
    {
        Position = position;

        groupFileStream->SetPosition(position);

        std::unique_ptr<WmoChunkHeader> header(groupFileStream->AllocateAndReadStruct<WmoChunkHeader>());

        Type = header->Type;
        Size = header->Size;
    }
}