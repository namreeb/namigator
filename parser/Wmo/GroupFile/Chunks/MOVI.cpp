#include "Wmo/GroupFile/Chunks/MOVI.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
MOVI::MOVI(size_t position, utility::BinaryStream* fileStream)
    : WmoGroupChunk(position, fileStream)
{
    Type = WmoGroupChunkType::MOVI;

    Indices.resize(Size / sizeof(std::uint16_t));

    fileStream->rpos(position + 8);
    fileStream->ReadBytes(&Indices[0], Size);
}
} // namespace input
} // namespace parser