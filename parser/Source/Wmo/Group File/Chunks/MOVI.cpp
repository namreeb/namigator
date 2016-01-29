#include "WMO/Group File/Chunks/MOVI.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
MOVI::MOVI(size_t position, utility::BinaryStream *fileStream) : WmoGroupChunk(position, fileStream)
{
    Type = WmoGroupChunkType::MOVI;

    Indices.resize(Size / sizeof(std::int16_t));

    fileStream->SetPosition(position + 8);
    fileStream->ReadBytes(&Indices[0], Size);
}
}
}