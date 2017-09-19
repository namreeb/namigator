#include "Adt/Chunks/MODF.hpp"

namespace parser
{
namespace input
{
MODF::MODF(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    if (!Size)
        return;

    Wmos.resize(Size / sizeof(WmoPlacement));
    reader->ReadBytes(&Wmos[0], Size);
}
}
}