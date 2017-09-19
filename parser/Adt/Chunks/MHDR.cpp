#include "Adt/AdtChunk.hpp"
#include "Adt/Chunks/MHDR.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"

namespace parser
{
namespace input
{
MHDR::MHDR(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MHDR)
        THROW("Expected (but did not find) MHDR type");

    if (Size != sizeof(MHDRInfo))
        THROW("MHDR chunk of incorrect size");
#endif

    reader->ReadBytes(&Offsets, sizeof(Offsets));
}
}
}