#include "Input/WMO/Root File/Chunks/MODN.hpp"

namespace parser
{
namespace input
{
MODN::MODN(unsigned int doodadNamesCount, long position, utility::BinaryStream *reader) : WmoRootChunk(position, reader)
{
    reader->SetPosition(position + 8);

    unsigned int currOffset = 0;

    for (unsigned int i = 0; i < doodadNamesCount; ++i)
    {
        unsigned char nextByte;

        while (!(nextByte = reader->Read<unsigned char>()))
            ++currOffset;

        std::string currFileName = reader->ReadCString();
        currFileName.insert(0, 1, nextByte);

        Names[currOffset] = currFileName;

        currOffset += (unsigned int)currFileName.length() + 1;
    }
}
}
}