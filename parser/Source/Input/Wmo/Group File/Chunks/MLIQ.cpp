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
        
        groupFileStream->ReadStruct(&Base);

        groupFileStream->Read<unsigned short>();

        Heights.reset(new Array2d<float>(Height, Width));
        for (unsigned int y = 0; y < Height; ++y)
            for (unsigned int x = 0; x < Width; ++x)
            {
                groupFileStream->Read<unsigned int>();  // unknown
                Heights->Set(y, x, groupFileStream->Read<float>());
            }
    }
}