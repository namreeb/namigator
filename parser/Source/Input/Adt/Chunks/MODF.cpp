#include "Input/ADT/Chunks/MODF.hpp"

namespace parser
{
namespace input
{
MODF::MODF(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    Wmos.resize(Size / WmoSize);
    reader->ReadBytes(&Wmos[0], Size);
}
}
}