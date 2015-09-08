#include "Misc.hpp"
#include "Input/Adt/AdtChunk.hpp"

namespace parser_input
{
    AdtChunk::AdtChunk(long position, utility::BinaryStream *reader)
    {
#ifdef DEBUG
        if (Position < 0)
            THROW("ADT chunk initialized with negative position");
#endif

        Position = position;

        reader->SetPosition(position);

        Type = reader->Read<unsigned int>();
        Size = reader->Read<unsigned int>();
    }
}