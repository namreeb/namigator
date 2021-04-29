#include "Adt/Chunks/MMDX.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"

#include <cstring>
#include <vector>

static_assert(sizeof(char) == 1, "char must be 8 bits");

namespace parser
{
namespace input
{
MMDX::MMDX(size_t position, utility::BinaryStream* reader)
    : AdtChunk(position, reader)
{
    if (Size == 0)
        return;

    reader->rpos(position + 8);

    std::vector<char> doodadNames(Size);
    reader->ReadBytes(&doodadNames[0], Size);

    char* p;

    for (p = &doodadNames[0]; *p == '\0'; ++p)
        ;

    do
    {
        DoodadNames.push_back(p);

        p = strchr(p, '\0');

        if (!p)
            break;

        p++;
    } while (p <= &doodadNames[Size - 1]);
}
} // namespace input
} // namespace parser