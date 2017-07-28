#include "Wmo/RootFile/Chunks/MODN.hpp"

#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
MODN::MODN(unsigned int doodadNamesCount, size_t position, utility::BinaryStream *reader) : WmoRootChunk(position, reader)
{
    reader->rpos(position + 8);

    unsigned int currOffset = 0;

    for (unsigned int i = 0; i < doodadNamesCount; ++i)
    {
        unsigned char nextByte;

        while ((nextByte = reader->Read<unsigned char>()) == 0)
            ++currOffset;

        std::string currFileName = reader->ReadString();
        currFileName.insert(0, 1, nextByte);

        Names[currOffset] = currFileName;

        currOffset += static_cast<unsigned int>(currFileName.length() + 1);
    }
}
}
}