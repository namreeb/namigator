#include "Misc.hpp"
#include "Input/ADT/Chunks/Subchunks/MCRD.hpp"

namespace parser_input
{
    MCRD::MCRD(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
#ifdef DEBUG
        if (Type != AdtChunkType::MCRD)
            THROW("Expected (but did not find) MCRD type");
#endif

        DoodadIndices.resize(Size / sizeof(unsigned int));

        reader->ReadBytes((void *)&DoodadIndices[0], Size);
    }
}