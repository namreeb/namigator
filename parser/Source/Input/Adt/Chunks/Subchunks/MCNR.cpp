#include "utility/Include/Misc.hpp"
#include "Input/ADT/Chunks/Subchunks/MCNR.hpp"

namespace parser
{
namespace input
{
MCNR::MCNR(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCNR)
        THROW("Expected (but did not find) MCNR type");
#endif

    reader->ReadBytes(&Entries, sizeof(Entries));
}
}
}