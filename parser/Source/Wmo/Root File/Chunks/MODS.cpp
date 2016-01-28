#include "Wmo/Root File/WmoRootChunk.hpp"
#include "Wmo/Root File/Chunks/MODS.hpp"

#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
MODS::MODS(unsigned int doodadSetsCount, size_t position, utility::BinaryStream *reader) : WmoRootChunk(position, reader), Count(doodadSetsCount)
{
    DoodadSets.resize(doodadSetsCount);

    reader->ReadBytes(&DoodadSets[0], sizeof(DoodadSetInfo)*doodadSetsCount);
}
}
}