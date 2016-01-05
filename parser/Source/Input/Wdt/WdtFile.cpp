#include "Input/WDT/WdtFile.hpp"
#include "Input/Wmo/Root File/WmoRootFile.hpp"
#include "utility/Include/BinaryStream.hpp"

namespace parser
{
namespace input
{
WdtFile::WdtFile(const std::string &path) : WowFile(path)
{
    Reader->SetPosition(GetChunkLocation("MPHD", 0) + 8);

    HasTerrain = !(Reader->Read<unsigned int>() & 0x1);

    Reader->SetPosition(GetChunkLocation("MAIN", 0) + 8);

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
        {
            int flag = Reader->Read<int>();

            // skip async object
            Reader->Slide(sizeof(int));

            HasAdt[y][x] = (flag & 1);
        }

    // for worlds with terrain, parsing stops here.  else, load single wmo
    if (HasTerrain)
        return;

    Reader->SetPosition(GetChunkLocation("MWMO", 0) + 8);

    std::string wmoName = Reader->ReadCString();

    Reader->SetPosition(GetChunkLocation("MODF", 0) + 8);

    WmoParserInfo wmoInfo;
    Reader->ReadBytes(&wmoInfo, sizeof(wmoInfo));

    Wmo.reset(new WmoRootFile(wmoName, &wmoInfo));
}
}
}