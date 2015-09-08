#include "Input/WMO/Root File/Chunks/MODD.hpp"

namespace parser_input
{
    MODD::MODD(long position, utility::BinaryStream *reader) : WmoRootChunk(position, reader)
    {
        reader->SetPosition(position + 8);
        Count = Size / sizeof(WmoDoodadInfo);

        Doodads.resize(Count);

        for (int i = 0; i < Count; ++i)
        {
            Doodads[i].Index = i;
            Doodads[i].DoodadInfo.reset(reader->AllocateAndReadStruct<WmoDoodadInfo>());
        }
    }
}