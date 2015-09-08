#include "Misc.hpp"
#include "Input/ADT/Chunks/MHDR.hpp"

namespace parser_input
{
    MHDR::MHDR(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
#ifdef DEBUG
        if (Type != AdtChunkType::MHDR)
            THROW("Expected (but did not find) MHDR type");

        if (Size != sizeof(MHDRInfo))
            THROW("MHDR chunk of incorrect size");
#endif

        Offsets.reset(reader->AllocateAndReadStruct<MHDRInfo>());
    }
}