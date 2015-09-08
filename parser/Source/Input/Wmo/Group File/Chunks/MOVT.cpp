#include "Input/WMO/Group File/Chunks/MOVT.hpp"

namespace parser_input
{
    MOVT::MOVT(long position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
    {
        Vertices.resize(Size / sizeof(Vertex));

        groupFileStream->ReadBytes(&Vertices[0], Size);
    }
}