#include "WMO/Group File/Chunks/MOVI.hpp"

namespace parser
{
namespace input
{
MOVI::MOVI(size_t position, utility::BinaryStream *fileStream) : WmoGroupChunk(position, fileStream)
{
    Type = WmoGroupChunkType::MOVI;

    Indices.resize(Size / sizeof(unsigned short));

    fileStream->SetPosition(position + 8);
    fileStream->ReadBytes(&Indices[0], Size);
}
}
}