#include "utility/Include/Exception.hpp"

#include "Adt/Chunks/Subchunks/MCNR.hpp"

namespace parser
{
namespace input
{
MCNR::MCNR(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCNR)
        THROW("Expected (but did not find) MCNR type");
#endif

    reader->ReadBytes(&Entries, sizeof(Entries));
}
}
}