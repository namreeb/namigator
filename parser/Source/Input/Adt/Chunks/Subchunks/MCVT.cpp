#include "utility/Include/Exception.hpp"
#include "Input/ADT/Chunks/Subchunks/MCVT.hpp"

namespace parser
{
namespace input
{
MCVT::MCVT(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
#ifdef DEBUG
    if (Type != AdtChunkType::MCVT)
        THROW("Expected (but did not find) MCVT type");
#endif

    reader->ReadBytes((void *)&Heights, sizeof(float)*(8*8 + 9*9));
}
}
}