#include "Adt/Chunks/MDDF.hpp"

#include "Doodad/DoodadPlacement.hpp"
#include "utility/BinaryStream.hpp"

namespace parser
{
namespace input
{
MDDF::MDDF(size_t position, utility::BinaryStream* reader)
    : AdtChunk(position, reader)
{
    if (!Size)
        return;

    Doodads.resize(Size / sizeof(DoodadPlacement));

    reader->rpos(position + 8);
    reader->ReadBytes(&Doodads[0], sizeof(DoodadPlacement) * Doodads.size());
}
} // namespace input
} // namespace parser