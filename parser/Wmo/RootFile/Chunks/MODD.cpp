#include "Wmo/RootFile/Chunks/MODD.hpp"

namespace parser
{
namespace input
{
MODD::MODD(size_t position, utility::BinaryStream* reader)
    : WmoRootChunk(position, reader), Count(Size / sizeof(WmoDoodadPlacement))
{
    if (!Count)
        return;

    reader->rpos(position + 8);

    Doodads.resize(Count);
    reader->ReadBytes(&Doodads[0], Size);
}
} // namespace input
} // namespace parser