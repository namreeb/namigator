#include "Doodad/DoodadPlacement.hpp"
#include "ADT/Chunks/MDDF.hpp"

#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
MDDF::MDDF(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    if (!Size)
        return;

    const unsigned int count = Size / sizeof(DoodadPlacement);
    Doodads.resize(count);

    reader->SetPosition(position + 8);
    reader->ReadBytes(&Doodads[0], sizeof(DoodadPlacement) * count);
}
}
}