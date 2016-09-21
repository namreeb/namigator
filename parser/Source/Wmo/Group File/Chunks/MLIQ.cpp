#include "WMO/Group File/Chunks/MLIQ.hpp"

namespace parser
{
namespace input
{
MLIQ::MLIQ(size_t position, utility::BinaryStream *groupFileStream) : WmoGroupChunk(position, groupFileStream)
{
    groupFileStream->SetPosition(position + 16);

    Width = groupFileStream->Read<unsigned int>();
    Height = groupFileStream->Read<unsigned int>();

    groupFileStream->ReadBytes(&Base, sizeof(Base));

    groupFileStream->Slide(2);

    Heights = std::make_unique<utility::Array2d<float>>(Height + 1, Width + 1);
    for (unsigned int y = 0; y <= Height; ++y)
        for (unsigned int x = 0; x <= Width; ++x)
        {
            groupFileStream->Slide(4);      // unknown
            Heights->Set(y, x, groupFileStream->Read<float>());
        }
}
}
}