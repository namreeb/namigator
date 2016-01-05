#include "Input/WMO/Root File/Chunks/MODS.hpp"

namespace parser
{
namespace input
{
MODS::MODS(unsigned int doodadSetsCount, long position, utility::BinaryStream *reader) : WmoRootChunk(position, reader), Count(doodadSetsCount)
{
    DoodadSets.resize(doodadSetsCount);

    reader->ReadBytes((void *)&DoodadSets[0], sizeof(DoodadSetInfo)*doodadSetsCount);
}
}
}