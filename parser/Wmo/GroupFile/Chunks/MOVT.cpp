#include "Wmo/GroupFile/Chunks/MOVT.hpp"

namespace parser
{
namespace input
{
MOVT::MOVT(size_t position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
{
    Vertices.resize(Size / sizeof(math::Vector3));

    groupFileStream->ReadBytes(&Vertices[0], Size);
}
}
}