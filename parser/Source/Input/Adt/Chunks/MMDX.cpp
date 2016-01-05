
#include "Input/ADT/Chunks/MMDX.hpp"
#include "Input/M2/DoodadFile.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Misc.hpp"

#include <list>
#include <vector>

namespace parser
{
namespace input
{
MMDX::MMDX(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    if (Size == 0)
        return;

    reader->SetPosition(position + 8);

    std::vector<char> doodadNames(Size);
    reader->ReadBytes(&doodadNames[0], Size);

    char *p;

    for (p = &doodadNames[0]; *p == '\0'; ++p);

    do
    {
        if (p >= &doodadNames[Size])
            THROW("Something that isnt supposed to be possible just happened.");

        DoodadNames.push_back(std::string(p));

        p = strchr(p, '\0');

        if (!p)
            break;

        p++;
    } while (p < &doodadNames[Size]);
}
}
}