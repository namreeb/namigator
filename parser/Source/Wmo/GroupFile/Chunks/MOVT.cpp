#include "Wmo/GroupFile/Chunks/MOVT.hpp"

namespace parser
{
namespace input
{
MOVT::MOVT(size_t position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
{
    Vertices.resize(Size / sizeof(utility::Vertex));

    groupFileStream->ReadBytes(&Vertices[0], Size);
}
}
}