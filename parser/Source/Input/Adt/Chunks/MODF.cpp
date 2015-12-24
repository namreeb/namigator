#include "Input/ADT/Chunks/MODF.hpp"

namespace parser_input
{
    MODF::MODF(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
        Wmos.resize(Size / WmoSize);
        reader->ReadBytes(&Wmos[0], Size);
    }
}