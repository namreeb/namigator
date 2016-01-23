#include "Input/WowFile.hpp"
#include "Output/MpqManager.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"

#include <algorithm>

namespace parser
{
namespace input
{
WowFile::WowFile(const ::std::string &path) : Reader(MpqManager::OpenFile(path))
{
    if (!Reader)
        THROW("MpqManager::OpenFile failed");
}

long WowFile::GetAnyChunkLocation(long startLoc)
{
    for (unsigned long l = startLoc; l < Reader->Length(); ++l)
    {
        Reader->SetPosition(l);
        char curChunkName[4];
        Reader->ReadBytes((void *)&curChunkName, 4);

        if (((curChunkName[0] >= 'A' && curChunkName[0] <= 'Z') || curChunkName[0] == '2') &&
            ((curChunkName[1] >= 'A' && curChunkName[1] <= 'Z') || curChunkName[1] == '2') &&
            ((curChunkName[2] >= 'A' && curChunkName[2] <= 'Z') || curChunkName[2] == '2') && curChunkName[3] == 'M')
            return l;
    }

    return -1L;
}

long WowFile::GetChunkLocation(const std::string &chunkName, int startLoc)
{
    unsigned long originalPosition = Reader->GetPosition();
    long firstChunk = GetAnyChunkLocation(startLoc);
    std::string chunkNameReversed = std::string(chunkName.rbegin(), chunkName.rend());
        
    for (long l = firstChunk; l < (long)Reader->Length(); )
    {
        if (l < 0)
            break;

        Reader->SetPosition(l);
        char curChunkName[5];
        Reader->ReadBytes(&curChunkName, 4);
        curChunkName[4] = '\0';

        if (!chunkNameReversed.compare(0, 4, curChunkName))
        {
            Reader->SetPosition(originalPosition);
            return l;
        }

        unsigned int size = Reader->Read<unsigned int>();
        l = GetAnyChunkLocation(l + 8 + size);
    }

    Reader->SetPosition(originalPosition);
    return -1L;
}
}
}