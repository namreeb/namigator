#include "ADT/Chunks/MWMO.hpp"
#include "utility/Include/Exception.hpp"

#include <vector>

static_assert(sizeof(char) == 1, "char must be 8 bit");

namespace parser
{
namespace input
{
MWMO::MWMO(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
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

        if ((p = strchr(p, '\0')) == nullptr)
            break;

        p++;
    } while (p <= &wmoNames[Size-1]);
}
}
}