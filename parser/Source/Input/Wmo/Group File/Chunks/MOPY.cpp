#include "Input/WMO/Group File/Chunks/MOPY.hpp"

namespace parser_input
{
    MOPY::MOPY(long position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
    {
        groupFileStream->SetPosition(position + 8);

        Type = WmoGroupChunkType::MOPY;

        TriangleCount = (int)Size / 2;

        Flags = new unsigned char[TriangleCount];
        MaterialId = new unsigned char[TriangleCount];

        for (int i = 0; i < TriangleCount; ++i)
        {
            Flags[i] = groupFileStream->Read<unsigned char>();
            MaterialId[i] = groupFileStream->Read<unsigned char>();
        }
    }

    MOPY::~MOPY()
    {
        delete Flags;
        delete MaterialId;
    }
}