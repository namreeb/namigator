#include <list>

#include "BinaryStream.hpp"
#include "Input/ADT/Chunks/MMDX.hpp"
#include "Input/M2/DoodadFile.hpp"

namespace parser_input
{
    MMDX::MMDX(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
        if (Size == 0)
            return;

        reader->SetPosition(position + 8);

        std::unique_ptr<char> doodadNames(new char[Size]);
        reader->ReadBytes(doodadNames.get(), Size);

        char *p;

        for (p = doodadNames.get(); *p == '\0'; ++p);

        do
        {
            if (p >= doodadNames.get() + Size)
                throw "Something that isnt supposed to be possible just happened.";

            DoodadNames.push_back(std::string(p));

            p = strchr(p, '\0');

            if (!p)
                break;

            p++;
        } while (p < doodadNames.get() + Size);
    }
}