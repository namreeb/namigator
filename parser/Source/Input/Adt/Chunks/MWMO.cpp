#include "Input/ADT/Chunks/MWMO.hpp"
#include "utility/Include/Misc.hpp"

#include <vector>

namespace parser
{
namespace input
{
MWMO::MWMO(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    Type = AdtChunkType::MWMO;

    if (!Size)
        return;

    reader->SetPosition(position + 8);

    std::vector<char> wmoNames(Size);
    reader->ReadBytes(&wmoNames[0], Size);

    char *p;

    for (p = &wmoNames[0]; *p == '\0'; ++p);

    do
    {
        WmoNames.push_back(std::string(p));

        if (!(p = strchr(p, '\0')))
            break;

        p++;
    } while (p <= &wmoNames[Size-1]);
}
}
}