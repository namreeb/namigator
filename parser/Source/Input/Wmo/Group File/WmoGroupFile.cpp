#include "Input/WMO/Group File/WmoGroupFile.hpp"
#include "utility/Include/Misc.hpp"

#include <memory>

namespace parser
{
namespace input
{
WmoGroupFile::WmoGroupFile(const std::string &path) : WowFile(path)
{
    char *cPath = new char[path.length()+1], *p;

    memcpy(cPath, path.c_str(), path.length() + 1);

    p = strrchr(cPath, '.');
    *p = '\0';
    p = strrchr(cPath, '_');
    *p++ = '\0';

    Index = atoi(p);

    delete[] cPath;

    // MOGP

    const long mogpOffset = GetChunkLocation("MOGP", 0xC);

    if (mogpOffset < 0)
        THROW("No MOGP chunk");

    // since all we want are flags + bounding box, we needn't bother creating an MOGP chunk class
    Reader->SetPosition(mogpOffset + 16);
    Flags = Reader->Read<unsigned int>();

    Reader->ReadBytes((void *)&Min, 3*sizeof(float));
    Reader->ReadBytes((void *)&Max, 3*sizeof(float));

    // MOPY

    const long mopyOffset = GetChunkLocation("MOPY", 0x58);

    if (mopyOffset < 0)
        THROW("No MOPY chunk");

    MaterialsChunk.reset(new MOPY(mopyOffset, Reader));

    // MOVI

    const long moviOffset = GetChunkLocation("MOVI", (int)mopyOffset + 4 + (int)MaterialsChunk->Size);

    if (moviOffset < 0)
        THROW("No MOVI chunk");

    IndicesChunk.reset(new MOVI(moviOffset, Reader));

    // MOVT

    const long movtOffset = GetChunkLocation("MOVT", (int)moviOffset + 4 + (int)IndicesChunk->Size);

    if (movtOffset < 0)
        THROW("No MOVT chunk");

    VerticesChunk.reset(new MOVT(movtOffset, Reader));

    // MLIQ

    const long mliqOffset = GetChunkLocation("MLIQ", 0x58);

    // not guarunteed to be present
    LiquidChunk.reset((mliqOffset >= 0 ? new MLIQ(mliqOffset, Reader) : nullptr));
}
}
}