#include "Input/WMO/Group File/Chunks/MOVI.hpp"

namespace parser_input
{
    MOVI::MOVI(long position, utility::BinaryStream *fileStream) : WmoGroupChunk(position, fileStream)
    {
        Type = WmoGroupChunkType::MOVI;

        // XXX - does sizeof(short) == 4 on x64?
        Indices.resize(Size / sizeof(unsigned short));

        fileStream->SetPosition(position + 8);
        fileStream->ReadBytes(&Indices[0], Size);
    }
}