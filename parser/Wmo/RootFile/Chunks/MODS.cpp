#include "Wmo/RootFile/Chunks/MODS.hpp"

#include "Wmo/RootFile/WmoRootChunk.hpp"
#include "utility/BinaryStream.hpp"

namespace parser
{
namespace input
{
MODS::MODS(unsigned int doodadSetsCount, size_t position,
           utility::BinaryStream* reader)
    : WmoRootChunk(position, reader), Count(doodadSetsCount)
{
    DoodadSets.resize(doodadSetsCount);

    reader->ReadBytes(&DoodadSets[0], sizeof(DoodadSetInfo) * doodadSetsCount);
}
} // namespace input
} // namespace parser