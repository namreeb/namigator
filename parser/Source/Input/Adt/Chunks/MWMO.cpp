#include "Input/ADT/Chunks/MWMO.hpp"

namespace parser_input
{
    MWMO::MWMO(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
        Type = AdtChunkType::MWMO;

        reader->SetPosition(position + 8);

        std::unique_ptr<char> wmoNames(new char[Size]);
        reader->ReadBytes(wmoNames.get(), Size);

        char *p;

        for (p = wmoNames.get(); *p == '\0'; ++p);

        do
        {
            if (p >= wmoNames.get() + Size)
                throw "Something that wasnt supposed to be possible has happened";

            WmoNames.push_back(std::string(p));

            if (!(p = strchr(p, '\0')))
                break;

            p++;
        } while (p < wmoNames.get() + Size);
    }
}