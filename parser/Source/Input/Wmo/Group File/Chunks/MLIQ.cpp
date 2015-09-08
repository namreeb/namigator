#include "Input/WMO/Group File/Chunks/MLIQ.hpp"

namespace parser_input
{
    MLIQ::MLIQ(long position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
    {
        groupFileStream->SetPosition(position + 8);

        Width = groupFileStream->Read<unsigned int>();
        Height = groupFileStream->Read<unsigned int>();

        groupFileStream->Read<unsigned int>();
        groupFileStream->Read<unsigned int>();
        
        groupFileStream->ReadBytes((void *)&Base, sizeof(float)*3);

        groupFileStream->Read<unsigned short>();

        Heights = new Array2d<float>(Height, Width);
        for (unsigned int y = 0; y < Height; ++y)
            for (unsigned int x = 0; x < Width; ++x)
            {
                Heights->Set(y, x, groupFileStream->Read<float>());
                groupFileStream->Read<float>();    // unknown
            }

        RenderMap = new Array2d<unsigned char>(Height, Width);
        groupFileStream->ReadBytes((void *)RenderMap->Data, Height * Width);
    }

    MLIQ::~MLIQ()
    {
        delete Heights;
        delete RenderMap;
    }
}