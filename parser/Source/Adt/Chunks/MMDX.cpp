#include "ADT/Chunks/MMDX.hpp"

#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"

#include <vector>

namespace parser
{
namespace input
{
MMDX::MMDX(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
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
        DoodadNames.push_back(p);

        p = strchr(p, '\0');

        if (!p)
            break;

        p++;
    } while (p <= &doodadNames[Size-1]);
}
}
}