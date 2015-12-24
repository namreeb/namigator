#include "Input/WMO/Root File/Chunks/MODD.hpp"

namespace parser_input
{
    MODD::MODD(long position, utility::BinaryStream *reader) : WmoRootChunk(position, reader), Count(Size/sizeof(WmoDoodadInfo))
    {
        reader->SetPosition(position + 8);

        Doodads.resize(Count);

        for (int i = 0; i < Count; ++i)
        {
            Doodads[i].Index = i;
            reader->ReadStruct(&Doodads[i].DoodadInfo);
        }
    }
}