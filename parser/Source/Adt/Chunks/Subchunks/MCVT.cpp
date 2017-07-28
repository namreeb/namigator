#include "utility/Include/Exception.hpp"
#include "Adt/Chunks/Subchunks/MCVT.hpp"

namespace parser
{
namespace input
{
MCVT::MCVT(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCVT)
        THROW("Expected (but did not find) MCVT type");
#endif

    reader->ReadBytes(&Heights, sizeof(Heights));
}
}
}