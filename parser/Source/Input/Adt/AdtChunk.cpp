#include "Input/Adt/AdtChunk.hpp"
#include "utility/Include/Misc.hpp"

namespace parser
{
namespace input
{
AdtChunk::AdtChunk(long position, utility::BinaryStream *reader) : Position(position)
{
#ifdef DEBUG
    if (Position < 0)
        THROW("ADT chunk initialized with negative position");
#endif

    reader->SetPosition(position);

    Type = reader->Read<unsigned int>();
    Size = reader->Read<unsigned int>();
}
}
}