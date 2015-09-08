#include "Misc.hpp"
#include "Input/ADT/Chunks/Subchunks/MCRW.hpp"

namespace parser_input
{
    MCRW::MCRW(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
#ifdef DEBUG
        if (Type != AdtChunkType::MCRW)
            THROW("Expected (but did not find) MCRW type");
#endif

        WmoIndices.resize(Size / sizeof(unsigned int));

        reader->ReadBytes((void *)&WmoIndices[0], Size);
    }
}