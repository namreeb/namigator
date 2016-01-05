#include "utility/Include/BinaryStream.hpp"
#include "Input/ADT/Chunks/MDDF.hpp"

namespace parser
{
namespace input
{
MDDF::MDDF(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    const unsigned int count = Size / sizeof(DoodadInfo);
    Doodads.resize(count);

    reader->SetPosition(position + 8);
    reader->ReadBytes(&Doodads[0], sizeof(DoodadInfo) * count);
}
}
}