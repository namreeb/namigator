#include "Adt/AdtChunk.hpp"

#include "utility/Include/Exception.hpp"

namespace parser
{
namespace input
{
AdtChunk::AdtChunk(size_t position, utility::BinaryStream *reader) : Position(position)
{
    reader->SetPosition(position);

    Type = reader->Read<unsigned int>();
    Size = reader->Read<unsigned int>();
}
}
}